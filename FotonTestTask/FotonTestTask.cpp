#include "FotonTestTask.h"

int main(int argc, char* argv[])
{
	const int inputFilePathIndex = 1;
	const int outputFilePathIndex = 2;
	const int nIndex = 3;

	try
	{
		auto now = high_resolution_clock::now();

		ThinBmp(argv[inputFilePathIndex],
				argv[outputFilePathIndex], 
				atoi(argv[nIndex]));

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Thinning has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
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

	auto readedInfo = ReadChangeWriteHeaders(input, output, n);

	std::vector<std::byte> inputRowBuffer(readedInfo.inputWidth * bytePerPx);
	std::vector<std::byte> outputRowBuffer(readedInfo.outputStride);

	for (size_t i = 0; i < readedInfo.inputHeight; i += n)
	{
		// Read row excluding padding bytes
		input.read((char*)inputRowBuffer.data(), readedInfo.inputWidth * bytePerPx);

		for (size_t j = 0; j < readedInfo.inputWidth; j += n)
		{
			// B
			outputRowBuffer[(j * bytePerPx) / n] = inputRowBuffer[j * bytePerPx];
			// G
			outputRowBuffer[(j * bytePerPx) / n + 1] = inputRowBuffer[j * bytePerPx + 1];
			// R
			outputRowBuffer[(j * bytePerPx) / n + 2] = inputRowBuffer[j * bytePerPx + 2];
		}

		// Writing thinned row with padding bytes
		output.write((char*)outputRowBuffer.data(), readedInfo.outputStride);

		// In input stream skip padding bytes and n-1 rows
		input.seekg(readedInfo.inputPaddingBytesCount + readedInfo.inputStride * (n - 1), 
					std::ios_base::cur);
	}
}

ReadedInfo ReadChangeWriteHeaders(std::ifstream& input, std::ofstream& output, int n)
{
	BmpFileHeader fileHeader{};
	DibHeader dibHeader{};
	ReadedInfo info{};

	input.read((char*)&fileHeader, sizeof(BmpFileHeader));
	input.read((char*)&dibHeader, sizeof(DibHeader));

	info.inputHeight = dibHeader.imageHeight;
	info.inputWidth = dibHeader.imageWidth;
	info.inputStride = (int64_t)(info.inputWidth * bytePerPx + 3) & ~3;
	info.inputPaddingBytesCount = info.inputStride - (int64_t)info.inputWidth * bytePerPx;

	int32_t outputHeight = ceil((float)info.inputHeight / n);
	int32_t outputWidth = ceil((float)info.inputWidth / n);
	info.outputStride = (int64_t)(outputWidth * bytePerPx + 3) & ~3;

	dibHeader.imageHeight = outputHeight;
	dibHeader.imageWidth = outputWidth;
	dibHeader.imageSize = info.outputStride * outputHeight;
	fileHeader.fileSize = dibHeader.imageSize + sizeof(BmpFileHeader) + sizeof(DibHeader);

	output.write((char*)&fileHeader, sizeof(BmpFileHeader));
	output.write((char*)&dibHeader, sizeof(DibHeader));

	return info;
}