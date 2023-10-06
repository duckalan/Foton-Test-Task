#include "TiffDownscaling.h"

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

				result.bitsPerSample = field.valueOffset;
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

				result.bitsPerSample = bitsPerSample[0];

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

	result.stripCount = result.stripOffsets.size();

	return result;
}

void WriteBmpHeaders(const RequiredTiffData& tiffData, std::ofstream& output)
{
	DibHeader dibHeader(tiffData);

	BmpFileHeader fileHeader;
	fileHeader.fileSize = dibHeader.imageSize + sizeof(BmpFileHeader) + sizeof(DibHeader);

	output.write((char*)&fileHeader, sizeof(BmpFileHeader));
	output.write((char*)&dibHeader, sizeof(DibHeader));
}

void CopyAvgValuesToDestRowBuffer(const vector<float>& avgValues, vector<uint8_t>& destRowBuffer) 
{
	for (size_t i = 0; i < avgValues.size(); i++)
	{
		destRowBuffer[i] = (uint8_t)(avgValues[i] + 0.5f);
	}
}

void DownscaleTiffWithAvgScaling(path inputFilePath, path outputFilePath, int n)
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

			SumWindowsInRow(srcRowBuffer, avgValuesBuffer, n);

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

			SumWindowsInRow(srcRowBuffer, avgValuesBuffer, n);

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

// адаптивное контрастирвоание
// гистрограмма для каждого из каналов (1 проход картинки перед обработкой)
// все значения случайной дискретной величины обрабатываем. Получаем количество величин
// это закон распределения. 
// у нас это яркость в канале. Число пикселец - общее значение возможных значений
// диапазон от 0 до 2^16 - 1. Массив под гистограмму, читаем всю картинку целиком
// перебираем каждый пиксель. Если в одном пикселе 1000, берём 1000й столбец и увеличиваем на 1
// по гистограмме авто контрастирование. У гистограммы площадь. Ширина столбца 1
// Высота - число пикселей содержащих эту яркость.
// Площадь - сумма числа пикселей, известна. Какой-то процент самых тёмных и ярких

// если min > max, принять max = min. Если они равны, учесть отсутствие делния на 0
// проверка за невыход за гистограмму.
// от min 0 до max 255 уравнение прямой, которое применяется для цветп
// f(min, max, color) п-квантили
void SumWindowsInRow(
	const vector<uint16_t>& srcRow,
	vector<float>& sumBuffer,
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

			// ВМЕСТО >> 2 ПРИМЕНЯТЬ ФУНКЦИЮ АДАПТИВНОГО КОНТРАСТИРОВАНИЯ ДЛЯ КАНАЛА 

			// B
			sumBuffer[x * ChannelCount / n] += srcRow[w + 2] >> 2;
			// G
			sumBuffer[x * ChannelCount / n + 1] += srcRow[w + 1] >> 2;
			// R
			sumBuffer[x * ChannelCount / n + 2] += srcRow[w] >> 2;
		}
	}

	// Суммирование значений оставшихся пикселей
	for (size_t w = lastWindowOffsetPx * ChannelCount;
		w < (lastWindowOffsetPx + remainingWidthPx) * ChannelCount;
		w += ChannelCount)
	{
		sumBuffer[lastWindowOffsetPx * ChannelCount / n] += srcRow[w + 2] >> 2;
		sumBuffer[lastWindowOffsetPx * ChannelCount / n + 1] += srcRow[w + 1] >> 2;
		sumBuffer[lastWindowOffsetPx * ChannelCount / n + 2] += srcRow[w] >> 2;
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