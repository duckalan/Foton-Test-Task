#include "FotonTestTask.h"

int main(int argc, char* argv[])
{
	const int inputFilePathIndex = 1;
	const int outputFilePathIndex = 2;
	const int nIndex = 3;

	try
	{
		auto now = high_resolution_clock::now();

		ThinBmpWith("C:\\Users\\Duck\\Desktop\\test\\5x5.bmp",
			argv[outputFilePathIndex],
			3);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Thinning has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}

void ThinBmpWith(const char* inputFilePath, const char* outputFilePath, int n)
{
	std::ifstream input(inputFilePath, std::ios::in | std::ios::binary);
	std::ofstream output(outputFilePath, std::ios::out | std::ios::binary);

	if (!input.is_open())
	{
		std::string errorMessage = "Can't find or open file with input path: ";
		throw std::invalid_argument(errorMessage + inputFilePath);
	}
	if (!output.is_open())
	{
		std::string errorMessage = "Can't save file with output path: ";
		throw std::invalid_argument(errorMessage + outputFilePath);
	}

	RequiredInfo info = ReadChangeWriteHeaders(input, output, n);

	std::vector<unsigned char> inputRowBuffer(info.inputWidth * BytePerPx);
	std::vector<float> avgValuesBuffer(info.outputWidth * BytePerPx);
	std::vector<std::byte> outputRowBuffer(info.outputStride);

	// Это количество пикселей в столбце в последнем окне
	int countRow = info.inputHeight % n;

	// Это количество пикселей в строке в последнем окне
	int countCol = info.inputWidth % n;

	for (size_t i = 0; i < info.inputHeight - countRow; i += n)
	{
		for (size_t j = 0; j < n; j++)
		{
			input.read((char*)inputRowBuffer.data(), info.inputWidth * BytePerPx);

			// Попиксельный проход вдоль строки по окну и подсчёт суммы
			for (size_t k = 0; k < info.inputWidth - countCol; k += n)
			{
				for (size_t w = k * BytePerPx; w < (k + n) * BytePerPx; w += BytePerPx)
				{
					avgValuesBuffer[k * BytePerPx / n] += inputRowBuffer[w];
					avgValuesBuffer[k * BytePerPx / n + 1] += inputRowBuffer[w + 1];
					avgValuesBuffer[k * BytePerPx / n + 2] += inputRowBuffer[w + 2];
				}
			}

			size_t k = info.inputWidth - countCol;

			// Суммирование значений оставшихся пикселей
			for (size_t w = k * BytePerPx; w < (k + countCol) * BytePerPx; w += BytePerPx)
			{
				avgValuesBuffer[k * BytePerPx / n] += inputRowBuffer[w];
				avgValuesBuffer[k * BytePerPx / n + 1] += inputRowBuffer[w + 1];
				avgValuesBuffer[k * BytePerPx / n + 2] += inputRowBuffer[w + 2];
			}
		}

		for (auto& f : avgValuesBuffer)
		{
			f /= 2;
		}

		//std::copy(begin(avgValuesBuffer), end(avgValuesBuffer), outputRowBuffer);
		//avgValuesBuffer.clear();
	}

	// ОБРАБОТКА ОСТАВШИХСЯ СТРОК
}


void ThinBmp(const char* inputFilePath, const char* outputFilePath, int n)
{
	std::ifstream input(inputFilePath, std::ios::in | std::ios::binary);
	std::ofstream output(outputFilePath, std::ios::out | std::ios::binary);

	if (!input.is_open())
	{
		std::string errorMessage = "Can't find or open file with input path: ";
		throw std::invalid_argument(errorMessage + inputFilePath);
	}
	if (!output.is_open())
	{
		std::string errorMessage = "Can't save file with output path: ";
		throw std::invalid_argument(errorMessage + outputFilePath);
	}

	RequiredInfo info = ReadChangeWriteHeaders(input, output, n);

	std::vector<std::byte> inputRowBuffer(info.inputWidth * BytePerPx);
	std::vector<std::byte> outputRowBuffer(info.outputStride);

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
		input.seekg(info.inputPaddingBytesCount + info.inputStride * (n - 1),
			std::ios_base::cur);
	}
}

RequiredInfo ReadChangeWriteHeaders(std::ifstream& input, std::ofstream& output, int n)
{
	BmpFileHeader fileHeader{};
	DibHeader dibHeader{};
	RequiredInfo info{};

	input.read((char*)&fileHeader, sizeof(BmpFileHeader));
	input.read((char*)&dibHeader, sizeof(DibHeader));

	info.inputHeight = dibHeader.imageHeight;
	info.inputWidth = dibHeader.imageWidth;
	info.inputStride = (int64_t)(info.inputWidth * BytePerPx + 3) & ~3;
	info.inputPaddingBytesCount = info.inputStride - (int64_t)info.inputWidth * BytePerPx;

	int32_t outputHeight = ceil((float)info.inputHeight / n);
	info.outputWidth = ceil((float)info.inputWidth / n);
	info.outputStride = (int64_t)(info.outputWidth * BytePerPx + 3) & ~3;

	dibHeader.imageHeight = outputHeight;
	dibHeader.imageWidth = info.outputWidth;
	dibHeader.imageSize = info.outputStride * outputHeight;
	fileHeader.fileSize = dibHeader.imageSize + sizeof(BmpFileHeader) + sizeof(DibHeader);

	output.write((char*)&fileHeader, sizeof(BmpFileHeader));
	output.write((char*)&dibHeader, sizeof(DibHeader));

	return info;
}