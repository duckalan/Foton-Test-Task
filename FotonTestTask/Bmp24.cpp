#include "Bmp24.h"
#include <cmath>

Bmp24::Bmp24(
	const BmpFileHeader& fileHeader,
	const DibHeader& dibHeader,
	const std::vector<Bgr24>& imageData)
{
	fileHeader_ = fileHeader;
	dibHeader_ = dibHeader;
	imageData_ = imageData;
}

Bmp24::Bmp24(const char* filePath)
{
	std::ifstream input(filePath, std::ios::binary);

	if (input.is_open())
	{
		input.read((char*)&fileHeader_, sizeof(BmpFileHeader));
		input.read((char*)&dibHeader_, sizeof(DibHeader));

		int bytePerPixel = dibHeader_.bitPerPixel / 8;
		int paddingBytesCount = dibHeader_.imageWidth % 4;

		auto pixelsCount = (dibHeader_.imageSize - paddingBytesCount * dibHeader_.imageHeight)
			/ bytePerPixel;

		imageData_ = std::vector<Bgr24>(pixelsCount);

		for (size_t i = 0; i < dibHeader_.imageHeight; i++)
		{
			input.read((char*)&imageData_[i * dibHeader_.imageWidth],
				dibHeader_.imageWidth * bytePerPixel - paddingBytesCount);
		}
	}
	else
	{
		std::string errorMessage = "Can't find or open file with input path: ";
		throw std::invalid_argument(errorMessage + filePath);
	}
}

Bmp24 Bmp24::ThinImage(int n) const
{
	BmpFileHeader newFileHeader(fileHeader_);
	DibHeader newDibHeader(dibHeader_);

	auto newHeight = (int32_t)ceil((float)dibHeader_.imageHeight / n);
	auto newWidth = (int32_t)ceil((float)dibHeader_.imageWidth / n);
	uint32_t newImageSize = newHeight * newWidth * sizeof(Bgr24) + (newWidth % 4) * newHeight;
	uint32_t newFileSize = newImageSize + sizeof(BmpFileHeader) + sizeof(DibHeader);

	newFileHeader.fileSize = newFileSize;

	newDibHeader.imageHeight = newHeight;
	newDibHeader.imageWidth = newWidth;
	newDibHeader.imageSize = newImageSize;

	std::vector<Bgr24> newImageData(newWidth * newHeight);
	// i == 507, j == 510
	for (size_t i = 0; i < dibHeader_.imageHeight; i += n)
	{
		for (size_t j = 0; j < dibHeader_.imageWidth; j += n)
		{
			newImageData[(i * newWidth + j) / n] = imageData_[i * dibHeader_.imageWidth + j];
		}
	}

	return Bmp24(newFileHeader, newDibHeader, newImageData);
}

void Bmp24::SaveToFile(const char* filePath) const
{
	std::ofstream output(filePath, std::ios::out | std::ios::binary);

	if (output.is_open())
	{
		output.write((char*)&fileHeader_, sizeof(BmpFileHeader));
		output.write((char*)&dibHeader_, sizeof(DibHeader));

		int bytePerPixel = dibHeader_.bitPerPixel / 8;
		int paddingBytesCount = dibHeader_.imageWidth % 4;
		std::vector<std::byte> paddingBytes(paddingBytesCount);

		for (size_t i = 0; i < dibHeader_.imageHeight; i++)
		{
			output.write((char*)&imageData_[i * dibHeader_.imageWidth],
				dibHeader_.imageWidth * sizeof(Bgr24));

			output.write((char*)paddingBytes.data(), paddingBytesCount);

			//output.close();
		}
	}
	else
	{
		std::string errorMessage = "Can't save file with output path: ";
		throw std::invalid_argument(errorMessage + filePath);
	}
}
