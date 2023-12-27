#include "ImageData.h"

ImageData::ImageData(std::ifstream& input, const BmpHeader& header)
{
	input.seekg(header.imageOffsetBytes);

	vec.resize(header.imageSizeBytes);
	input.read((char*)vec.data(), header.imageSizeBytes);

	widthPx = header.imageWidthPx;
	heightPx = header.imageHeightPx;
	paddingBytesCount = ((header.imageWidthPx * 3 + 3) & ~3) - header.imageWidthPx * 3;
}

uint8_t ImageData::GetB(int x, int y) const
{
	return vec[(y * widthPx + x) * 3 + paddingBytesCount * y];
}

uint8_t ImageData::GetG(int x, int y) const
{
	return vec[(y * widthPx + x) * 3 + paddingBytesCount * y + 1];
}

uint8_t ImageData::GetR(int x, int y) const
{
	return vec[(y * widthPx + x) * 3 + paddingBytesCount * y + 2];
}