#include <fstream>
#include "BmpDownscaling.h"
#include "BmpFileHeader.h"
#include "DibHeader.h"


void DownscaleBmpWithPixelSkipping(
	path inputFilePath,
	path outputFilePath,
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

	RequiredBmpValues info = ReadChangeWriteHeaders(input, output, n);

	vector<std::byte> inputRowBuffer(info.inputWidth * BytePerPx);
	vector<std::byte> outputRowBuffer(info.outputStride);

	for (size_t i = 0; i < info.inputHeight; i += n)
	{
		// Read row excluding padding bytes
		input.read((char*)inputRowBuffer.data(), info.inputWidth * BytePerPx);

		for (size_t j = 0; j < info.inputWidth; j += n)
		{
			// B
			outputRowBuffer[(j * BytePerPx) / n] = inputRowBuffer[j * BytePerPx];
			// G
			outputRowBuffer[(j * BytePerPx) / n + 1] = inputRowBuffer[j * BytePerPx + 1];
			// R
			outputRowBuffer[(j * BytePerPx) / n + 2] = inputRowBuffer[j * BytePerPx + 2];
		}

		// Writing thinned row with padding bytes
		output.write((char*)outputRowBuffer.data(), info.outputStride);

		// In input stream skip padding bytes and n-1 rows
		input.seekg(
			info.inputPaddingBytesCount + info.inputStride * (n - 1),
			std::ios_base::cur);
	}
}

void DownscaleBmpWithAvgScailing(
	path inputFilePath,
	path outputFilePath,
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

	RequiredBmpValues info = ReadChangeWriteHeaders(input, output, n);

	vector<uint8_t> inputRowBuffer(info.inputWidth * BytePerPx);
	vector<uint8_t> outputRowBuffer(info.outputStride);
	vector<float> avgValuesBuffer(info.outputWidth * BytePerPx);

	// ���������� �������� � ������� � ��������� ����
	int remainingHeight = info.inputHeight % n;

	// ���������� �������� � ������ � ��������� ����
	int remainingWidth = info.inputWidth % n;

	for (size_t i = 0; i < info.inputHeight - remainingHeight; i += n)
	{
		// ���������� ����
		for (size_t j = 0; j < n; j++)
		{
			input.read((char*)inputRowBuffer.data(), info.inputWidth * BytePerPx);
			input.seekg(info.inputPaddingBytesCount, std::ios_base::cur);

			SumWindowsInRow(
				inputRowBuffer,
				avgValuesBuffer,
				info.inputWidth,
				n);
		}

		/*FindAvgValuesInSumBuffer(
			avgValuesBuffer,
			info.inputWidth,
			info.inputHeight,
			false,
			n);*/

		std::copy(begin(avgValuesBuffer), end(avgValuesBuffer), begin(outputRowBuffer));

		output.write((char*)outputRowBuffer.data(), info.outputStride);

		// ��������� ������� ������� ��������
		std::fill(begin(avgValuesBuffer), end(avgValuesBuffer), 0.f);
	}

	// ��������� ���������� �����
	if (remainingHeight != 0)
	{
		for (size_t j = 0; j < remainingHeight; j++)
		{
			input.read((char*)inputRowBuffer.data(), info.inputWidth * BytePerPx);
			input.seekg(info.inputPaddingBytesCount, std::ios_base::cur);

			SumWindowsInRow(
				inputRowBuffer,
				avgValuesBuffer,
				info.inputWidth,
				n);
		}

		/*FindAvgValuesInSumBuffer(
			avgValuesBuffer,
			info.inputWidth,
			info.inputHeight,
			true,
			n);*/

		std::copy(begin(avgValuesBuffer), end(avgValuesBuffer), begin(outputRowBuffer));

		output.write((char*)outputRowBuffer.data(), info.outputStride);

		std::fill(begin(avgValuesBuffer), end(avgValuesBuffer), 0.f);
	}
}

void SumWindowsInRow(
	const vector<unsigned char>& inputRow,
	vector<float>& sumBuffer,
	int32_t inputImageWidthInPixels,
	int n)
{
	int remainingWidth = inputImageWidthInPixels % n;

	// ������������ ������ ����� ������ �� ����� � ������� �����
	for (size_t x = 0;
		x < inputImageWidthInPixels - remainingWidth;
		x += n)
	{
		for (size_t w = x * BytePerPx; w < (x + n) * BytePerPx; w += BytePerPx)
		{
			sumBuffer[x * BytePerPx / n] += inputRow[w];
			sumBuffer[x * BytePerPx / n + 1] += inputRow[w + 1];
			sumBuffer[x * BytePerPx / n + 2] += inputRow[w + 2];
		}
	}

	size_t lastWindowOffset = inputImageWidthInPixels - remainingWidth;

	// ������������ �������� ���������� ��������
	for (size_t w = lastWindowOffset * BytePerPx;
		w < (lastWindowOffset + remainingWidth) * BytePerPx;
		w += BytePerPx)
	{
		sumBuffer[lastWindowOffset * BytePerPx / n] += inputRow[w];
		sumBuffer[lastWindowOffset * BytePerPx / n + 1] += inputRow[w + 1];
		sumBuffer[lastWindowOffset * BytePerPx / n + 2] += inputRow[w + 2];
	}
}

//void FindAvgValuesInSumBuffer(
//	vector<float>& sumBuffer,
//	int32_t srcW, int32_t srcH,
//	bool isTopEdge,
//	int n)
//{
//	// ���������� �������� � ������ � ��������� ����
//	int remainingWidth = srcW % n;
//
//	int windowSquare;
//	int lastWindowSquare;
//
//	if (isTopEdge)
//	{
//		// ���������� �������� � ������� � ��������� ����
//		int remainingHeight = srcH % n;
//
//		// ��������� ������ ����
//		windowSquare = n * remainingHeight;
//		lastWindowSquare = remainingWidth * remainingHeight;
//	}
//	else
//	{
//		windowSquare = n * n;
//		lastWindowSquare = remainingWidth * n;
//	}
//
//	// ���������� ������� ��������
//	for (size_t k = 0;
//		k < (size_t)srcW - remainingWidth;
//		k += n)
//	{
//		sumBuffer[k * BytePerPx / n] /= windowSquare;
//		sumBuffer[k * BytePerPx / n + 1] /= windowSquare;
//		sumBuffer[k * BytePerPx / n + 2] /= windowSquare;
//	}
//
//	// ���������� ������� �������� � ���������� ����� ��������
//	if (remainingWidth != 0)
//	{
//		size_t lastPixelOffset = (size_t)srcW - remainingWidth;
//		
//		sumBuffer[lastPixelOffset * BytePerPx / n] /= lastWindowSquare;
//		sumBuffer[lastPixelOffset * BytePerPx / n + 1] /= lastWindowSquare;
//		sumBuffer[lastPixelOffset * BytePerPx / n + 2] /= lastWindowSquare;
//	}
//}

RequiredBmpValues ReadChangeWriteHeaders(
	std::ifstream& input,
	std::ofstream& output,
	int n) 
{
	BmpFileHeader fileHeader{};
	DibHeader dibHeader{};
	RequiredBmpValues info{};

	input.read((char*)&fileHeader, sizeof(BmpFileHeader));
	input.read((char*)&dibHeader, sizeof(DibHeader));

	info.inputHeight = dibHeader.imageHeight;
	info.inputWidth = dibHeader.imageWidth;
	info.inputStride = (int64_t)(info.inputWidth * BytePerPx + 3) & ~3;
	info.inputPaddingBytesCount = info.inputStride - (int64_t)info.inputWidth * BytePerPx;

	int32_t outputHeight = (info.inputHeight + n - 1) / n;
	info.outputWidth = (info.inputWidth + n - 1) / n;
	info.outputStride = (int64_t)(info.outputWidth * BytePerPx + 3) & ~3;

	dibHeader.imageHeight = outputHeight;
	dibHeader.imageWidth = info.outputWidth;
	dibHeader.imageSize = info.outputStride * outputHeight;
	fileHeader.fileSize = dibHeader.imageSize + sizeof(BmpFileHeader) + sizeof(DibHeader);

	output.write((char*)&fileHeader, sizeof(BmpFileHeader));
	output.write((char*)&dibHeader, sizeof(DibHeader));

	return info;
}