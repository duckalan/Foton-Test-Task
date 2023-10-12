#include "TiffDownscaling.h"
#include <algorithm>
#include <numeric>

void DownscaleTiffWithAvgScaling(
	path inputFilePath, 
	path outputFilePath, 
	float minContrastBorder,
	float maxContrastBorder, 
	int n)
{
	std::ifstream input(inputFilePath, std::ios::in | std::ios::binary);
	std::ofstream output(outputFilePath, std::ios::out | std::ios::binary);

	if (!input.is_open())
	{
		std::string errorMessage = "Can't find or open file with input path: ";
		throw std::invalid_argument(errorMessage + inputFilePath.string());
	}
	if (!output.is_open())
	{
		std::string errorMessage = "Can't save file with output path: ";
		throw std::invalid_argument(errorMessage + outputFilePath.string());
	}

	RequiredTiffData tiffData = ReadTiff(input);

	tiffData.destWidthPx = (tiffData.srcWidthPx + n - 1) / n;
	tiffData.destLengthPx = (tiffData.srcLengthPx + n - 1) / n;
	tiffData.destStrideBytes = (tiffData.destWidthPx * BmpBytePerPx + 3) & ~3;

	WriteBmpHeaders(tiffData, output);

	// uint16_t т.к. на канал приходится по 2 байта
	vector<uint16_t> srcRowBuffer(tiffData.srcWidthPx * ChannelCount);
	vector<uint8_t> destRowBuffer(tiffData.destStrideBytes);
	vector<float> avgValuesBuffer(tiffData.destWidthPx * ChannelCount);

	// без auto
	auto contrastingFuncs = BuildContrastingFuncs(
		input, tiffData, 
		minContrastBorder, 
		maxContrastBorder);

	int remainingLengthPx = tiffData.srcLengthPx % n;
	int remainingWidthPx = tiffData.srcWidthPx % n;

	int rowInStripCounter = 0;
	int stripCounter = 1;
	int bmpRowCounter = tiffData.destLengthPx;

	// Переход к первой полосе
	input.seekg(tiffData.stripOffsets[0]);

	// Переход к позиции для начала записи с конца
	output.seekp(BmpImageOffsetBytes + (bmpRowCounter--) * tiffData.destStrideBytes);

	for (size_t y = 0; y < tiffData.srcLengthPx - remainingLengthPx; y += n)
	{
		for (size_t j = 0; j < n; j++)
		{
			// Смещение на начало другой полосы
			if (rowInStripCounter == tiffData.rowsPerStrip)
			{
				input.seekg(tiffData.stripOffsets[stripCounter++]);
				rowInStripCounter = 0;
			}

			input.read((char*)srcRowBuffer.data(), tiffData.srcWidthPx * TiffBytePerPx);

			SumWindowsInRow(srcRowBuffer, avgValuesBuffer, contrastingFuncs, n);

			rowInStripCounter++;
		}

		CalculateAvgValuesInSumBuffer(
			avgValuesBuffer,
			tiffData.srcWidthPx,
			tiffData.srcLengthPx,
			false,
			n);

		CopyAvgValuesToDestRowBuffer(avgValuesBuffer, destRowBuffer);

		output.write((char*)destRowBuffer.data(), destRowBuffer.size());

		if (bmpRowCounter >= 0)
		{
			output.seekp(BmpImageOffsetBytes + (bmpRowCounter--) * tiffData.destStrideBytes);
		}

		std::fill(begin(avgValuesBuffer), end(avgValuesBuffer), 0.f);
	}

	// Обработка оставшихся строк
	if (remainingLengthPx != 0)
	{
		for (size_t j = 0; j < remainingLengthPx; j++)
		{
			if (rowInStripCounter == tiffData.rowsPerStrip)
			{
				input.seekg(tiffData.stripOffsets[stripCounter++]);
				rowInStripCounter = 0;
			}

			input.read((char*)srcRowBuffer.data(), tiffData.srcWidthPx * TiffBytePerPx);

			SumWindowsInRow(srcRowBuffer, avgValuesBuffer, contrastingFuncs, n);

			rowInStripCounter++;
		}

		CalculateAvgValuesInSumBuffer(
			avgValuesBuffer,
			tiffData.srcWidthPx,
			tiffData.srcLengthPx,
			true,
			n);

		CopyAvgValuesToDestRowBuffer(avgValuesBuffer, destRowBuffer);

		output.write((char*)destRowBuffer.data(), destRowBuffer.size());

		std::fill(begin(avgValuesBuffer), end(avgValuesBuffer), 0.f);
	}
}


RequiredTiffData ReadTiff(std::ifstream& input)
{
	int32_t tiffIdentifier = 0;
	input.read((char*)&tiffIdentifier, sizeof(uint32_t));

	if (tiffIdentifier != LittleEndianTiffIdentifier)
	{
		throw std::invalid_argument("Can't work with big-endian tiff format");
	}

	uint32_t ifdOffset = 0;
	input.read((char*)&ifdOffset, sizeof(uint32_t));

	input.seekg(ifdOffset);

	uint16_t fieldCount = 0;
	input.read((char*)&fieldCount, sizeof(uint16_t));

	RequiredTiffData result{};
	TiffField field{};

	for (size_t i = 0; i < fieldCount; i++)
	{
		input.read((char*)&field, sizeof(field));

		switch (field.tag)
		{
		case ImageWidthTag:
			result.srcWidthPx = field.valueOffset;
			break;

		case ImageLengthTag:
			result.srcLengthPx = field.valueOffset;
			break;

		case BitsPerSampleTag:
		{
			if (field.count == 1)
			{
				if (field.valueOffset != 16)
				{
					throw std::invalid_argument("Can't work with images with image depth not 16 bit per sample ");
				}
			}
			else
			{
				auto lastPosition = input.tellg();

				input.seekg(field.valueOffset);

				uint16_t bitsPerSample[3]{};
				input.read((char*)bitsPerSample, sizeof(bitsPerSample));

				for (size_t j = 0; j < 3; j++)
				{
					if (bitsPerSample[j] != 16)
					{
						throw std::invalid_argument("Can't work with images with image depth not 16 bit per sample ");
					}
				}

				input.seekg(lastPosition);
			}
			break;
		}

		case CompressionTag:
			if (field.valueOffset != ImageWithoutCompression)
			{
				throw std::invalid_argument("Can't work with compressed images");
			}
			break;

		case StripOffsetsTag:
			if (field.count == 1)
			{
				result.stripOffsets = std::vector<uint32_t>(1);
				input.read((char*)result.stripOffsets.data(), sizeof(uint32_t));
			}
			else
			{
				auto lastPosition = input.tellg();

				input.seekg(field.valueOffset);

				result.stripOffsets = std::vector<uint32_t>(field.count);
				input.read((char*)result.stripOffsets.data(), field.count * sizeof(uint32_t));

				input.seekg(lastPosition);
			}
			break;

		case RowsPerStripTag:
			result.rowsPerStrip = field.valueOffset;
			break;

		default:
			break;
		}
	}

	return result;
}


void WriteBmpHeaders(
	const RequiredTiffData& tiffData, 
	std::ofstream& output)
{
	DibHeader dibHeader(tiffData);

	BmpFileHeader fileHeader;
	fileHeader.fileSize = dibHeader.imageSize + sizeof(BmpFileHeader) + sizeof(DibHeader);

	output.write((char*)&fileHeader, sizeof(BmpFileHeader));
	output.write((char*)&dibHeader, sizeof(DibHeader));
}


void SumWindowsInRow(
	const vector<uint16_t>& srcRow,
	vector<float>& sumBuffer,
	const array<function<float(float)>, 3>& contrastingFuncs,
	int n)
{
	int32_t srcWidthPx = srcRow.size() / ChannelCount;
	int remainingWidthPx = srcWidthPx % n;
	size_t lastWindowOffsetPx = static_cast<size_t>(srcWidthPx) - remainingWidthPx;

	for (size_t x = 0; x < lastWindowOffsetPx; x += n)
	{
		for (size_t w = x * ChannelCount;
			w < (x + n) * ChannelCount;
			w += ChannelCount)
		{
			// Так как в tiff порядок rgb, здесь необходимо
			// записывать в буфер в обратном порядке

			// В конце контрастирование
			// B
			sumBuffer[x * ChannelCount / n] += contrastingFuncs[2](srcRow[w + 2]);
			// G
			sumBuffer[x * ChannelCount / n + 1] += contrastingFuncs[1](srcRow[w + 1]);
			// R
			sumBuffer[x * ChannelCount / n + 2] += contrastingFuncs[0](srcRow[w]);
		}
	}

	// Суммирование значений оставшихся пикселей
	for (size_t w = lastWindowOffsetPx * ChannelCount;
		w < (lastWindowOffsetPx + remainingWidthPx) * ChannelCount;
		w += ChannelCount)
	{
		sumBuffer[lastWindowOffsetPx * ChannelCount / n] += contrastingFuncs[2](srcRow[w + 2]);
		sumBuffer[lastWindowOffsetPx * ChannelCount / n + 1] += contrastingFuncs[1](srcRow[w + 1]);
		sumBuffer[lastWindowOffsetPx * ChannelCount / n + 2] += contrastingFuncs[0](srcRow[w]);
	}
}


void CalculateAvgValuesInSumBuffer(
	vector<float>& sumBuffer,
	int32_t srcWidthPx, int32_t srcLengthPx,
	bool isBottomEdge,
	int n)
{
	// Количество пикселей в строке в последнем окне
	int remainingWidthPx = srcWidthPx % n;

	int windowSquare;
	int lastWindowSquare;

	if (isBottomEdge)
	{
		// Количество пикселей в столбце в последнем окне
		int remainingLengthPx = srcLengthPx % n;

		windowSquare = n * remainingLengthPx;
		lastWindowSquare = remainingWidthPx * remainingLengthPx;
	}
	else
	{
		windowSquare = n * n;
		lastWindowSquare = remainingWidthPx * n;
	}

	// Нахождение средних значений
	for (size_t k = 0;
		k < (size_t)srcWidthPx - remainingWidthPx;
		k += n)
	{
		// после деления контрастирование
		sumBuffer[k * ChannelCount / n] /= windowSquare;
		sumBuffer[k * ChannelCount / n + 1] /= windowSquare;
		sumBuffer[k * ChannelCount / n + 2] /= windowSquare;
	}

	// Нахождение средних значений у оставшейся части пикселей
	if (remainingWidthPx != 0)
	{
		size_t lastPixelOffset = (size_t)srcWidthPx - remainingWidthPx;

		sumBuffer[lastPixelOffset * ChannelCount / n] /= lastWindowSquare;
		sumBuffer[lastPixelOffset * ChannelCount / n + 1] /= lastWindowSquare;
		sumBuffer[lastPixelOffset * ChannelCount / n + 2] /= lastWindowSquare;
	}
}


void CopyAvgValuesToDestRowBuffer(
	const vector<float>& avgValues, 
	vector<uint8_t>& destRowBuffer)
{
	for (size_t i = 0; i < avgValues.size(); i++)
	{
		// Здесь контрастирование с uint8_t и округлением
		destRowBuffer[i] = (uint8_t)(avgValues[i] + 0.5f);
	}
}


array<function<float(float)>, 3> BuildContrastingFuncs(
	std::ifstream& input,
	const RequiredTiffData& tiffData,
	float minBorder, float maxBorder)
{
	vector<uint16_t> srcRowBuffer(tiffData.srcWidthPx * ChannelCount);
	int rowInStripCounter = 0;
	int stripCounter = 1;

	const uint32_t histogramSquare = tiffData.srcWidthPx * tiffData.srcLengthPx;

	vector<uint32_t> rHistogram(HistogramSize);
	vector<uint32_t> gHistogram(HistogramSize);
	vector<uint32_t> bHistogram(HistogramSize);

	input.seekg(tiffData.stripOffsets[0]);

	// Расчёт гистограмм
	for (size_t y = 0; y < tiffData.srcLengthPx; y++)
	{
		// Смещение на начало другой полосы
		if (rowInStripCounter == tiffData.rowsPerStrip)
		{
			input.seekg(tiffData.stripOffsets[stripCounter++]);
			rowInStripCounter = 0;
		}

		input.read((char*)srcRowBuffer.data(), tiffData.srcWidthPx * TiffBytePerPx);

		for (size_t x = 0; x < tiffData.srcWidthPx * ChannelCount; x += ChannelCount)
		{
			rHistogram[srcRowBuffer[x]] += 1;
			gHistogram[srcRowBuffer[x + 1]] += 1;
			bHistogram[srcRowBuffer[x + 2]] += 1;
		}

		rowInStripCounter++;
	}

	return array<function<float(float)>, 3> 
	{
		BuildContrastingFunc(rHistogram, histogramSquare, minBorder, maxBorder),
		BuildContrastingFunc(gHistogram, histogramSquare, minBorder, maxBorder),
		BuildContrastingFunc(bHistogram, histogramSquare, minBorder, maxBorder),
	};
}



function<float(float)> BuildContrastingFunc(
	const vector<uint32_t> histogram,
	uint32_t histogramSquare,
	float minBorder, float maxBorder)
{
	const float minSquare = histogramSquare * minBorder;
	const float maxSquare = histogramSquare * maxBorder;

	int32_t squareSum = 0;
	int32_t pseudoMin = 0;
	while (squareSum < minSquare && pseudoMin < HistogramSize)
	{
		squareSum += histogram[pseudoMin++];
	}

	squareSum = histogramSquare;
	int32_t pseudoMax = HistogramSize - 1;
	while (squareSum > maxSquare && pseudoMax > 0)
	{
		squareSum -= histogram[pseudoMax--];
	}

	if (pseudoMin >= pseudoMax)
	{
		// если больше, то можно поменять местами
		// если равенство, то макс увеличить на 1, мин на 1 уменьшить или т.п.
		return [](float value)
			{
				return 127.f;
			};
	}

	// либо возвращать мин и макс, и отдельно функцию 
	// либо отдельную функцию с 3 параметрами с
	// сразу uint8_t и округление
	return [pseudoMin, pseudoMax](float value)
		{
			return std::clamp((value - pseudoMin) * 255.f / (pseudoMax - pseudoMin), 0.f, 255.f);
		};
}