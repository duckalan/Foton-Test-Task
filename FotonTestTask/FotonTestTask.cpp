#include "FotonTestTask.h"

// Read bmp with color depth 24 bits per pixel, 
// thin it by N times, save thinned image in new bmp

int main(int argc, char* argv[])
{
	const int inputFilePathIndex = 1;
	const int outputFilePathIndex = 2;
	const int nIndex = 3;

	auto in = "H:\\test3.bmp";
	auto out = "H:\\testO.bmp";
	try
	{
		auto now = high_resolution_clock::now();

		ThinBmpImage(in, out, 5);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Thinning has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}

void ThinBmpImage(const char* inputFilePath, const char* outputFilePath, int n)
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

	BmpFileHeader fileHeader{};
	DibHeader dibHeader{};

	input.read((char*)&fileHeader, sizeof(BmpFileHeader));
	input.read((char*)&dibHeader, sizeof(DibHeader));

	auto originalHeight = dibHeader.imageHeight;
	auto originalWidth = dibHeader.imageWidth;
	int originalStride = ((originalWidth * dibHeader.bitPerPixel + 31) & ~31) >> 3;
	int originalPaddigBytesCount = originalStride - originalWidth * sizeof(Bgr24);

	auto newHeight = (int32_t)ceil((float)originalHeight / n);
	auto newWidth = (int32_t)ceil((float)originalWidth / n);
	int newStride = ((newWidth * dibHeader.bitPerPixel + 31) & ~31) >> 3;
	int newPaddingBytesCount = newStride - newWidth * sizeof(Bgr24);
	std::vector<std::byte> newPaddingBytes(newPaddingBytesCount);

	uint32_t newImageSize = newStride * newHeight;
	uint32_t newFileSize = newImageSize + sizeof(BmpFileHeader) + sizeof(DibHeader);

	fileHeader.fileSize = newFileSize;
	dibHeader.imageHeight = newHeight;
	dibHeader.imageWidth = newWidth;
	dibHeader.imageSize = newImageSize;

	output.write((char*)&fileHeader, sizeof(BmpFileHeader));
	output.write((char*)&dibHeader, sizeof(DibHeader));

	for (size_t i = 0; i < originalHeight; i += n)
	{
		std::vector<Bgr24> originalRowBuffer(originalWidth);

		// Read row excluding padding bytes
		input.read((char*)originalRowBuffer.data(), originalWidth * sizeof(Bgr24));

		// Write thinned row
		for (size_t j = 0; j < originalWidth; j += n)
		{
			output.write((char*)&originalRowBuffer[j], sizeof(Bgr24));
		}

		// Write padding bytes
		output.write((char*)newPaddingBytes.data(), newPaddingBytesCount);

		// Skip padding bytes and n-1 rows in input stream
		input.seekg(originalPaddigBytesCount + originalStride * (n - 1), std::ios_base::cur);
	}
}