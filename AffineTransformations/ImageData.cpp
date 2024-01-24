#include "ImageData.h"

void ImageData::MirrorRowEdges(
	int rowOffsetBytes, 
	int originalWidthPx, 
	uint32_t pxToMirrorCount)
{
	uint8_t* rowStart = &image[rowOffsetBytes];

	// Левый край
	for (int x = 0; x < pxToMirrorCount; x++)
	{
		int mirrorX = pxToMirrorCount * 2 - 1 - x;

		*(rowStart + x * 3) = *(rowStart + mirrorX * 3);
		*(rowStart + x * 3 + 1) = *(rowStart + mirrorX * 3 + 1);
		*(rowStart + x * 3 + 2) = *(rowStart + mirrorX * 3 + 2);
	}

	// Правый край
	for (int x = originalWidthPx + pxToMirrorCount;
		x < originalWidthPx + pxToMirrorCount * 2;
		x++)
	{
		int mirrorX = 2 * (originalWidthPx + pxToMirrorCount) - x - 1;

		*(rowStart + x * 3) = *(rowStart + mirrorX * 3);
		*(rowStart + x * 3 + 1) = *(rowStart + mirrorX * 3 + 1);
		*(rowStart + x * 3 + 2) = *(rowStart + mirrorX * 3 + 2);
	}
}

void ImageData::ExtendRowEdges(
	int rowOffsetBytes, 
	int originalWidthPx, 
	uint32_t pxToExtendCount)
{
	uint8_t* rowStart = &image[rowOffsetBytes];
	uint8_t* leftEdge = rowStart + pxToExtendCount * 3;
	uint8_t* rightEdge = rowStart + pxToExtendCount * 3 + (originalWidthPx * 3) - 3;

	for (size_t x = 0; x < pxToExtendCount; x++)
	{
		*(rowStart + x * 3) = *(leftEdge);
		*(rowStart + x * 3 + 1) = *(leftEdge + 1);
		*(rowStart + x * 3 + 2) = *(leftEdge + 2);

		*(rightEdge + x * 3 + 3) = *(rightEdge);
		*(rightEdge + x * 3 + 4) = *(rightEdge + 1);
		*(rightEdge + x * 3 + 5) = *(rightEdge + 2);
	}
}

ImageData::ImageData(
	std::ifstream& input, 
	const BmpHeader& header, 
	InterpolationType interpolationType)
{
	if (interpolationType == InterpolationType::BiCubic ||
		interpolationType == InterpolationType::Lanczos2)
	{
		// Отразить на границе 1 пиксель
		extendedPxCount = 1;
	}
	else if (interpolationType == InterpolationType::Lanczos3)
	{
		// Отразить на границе 2 пикселя
		extendedPxCount = 2;
	}
	else
	{
		extendedPxCount = 0;
	}

	// extendedPxCount + 1 = радиус окна
	if (header.imageWidthPx < (extendedPxCount + 1) ||
		header.imageHeightPx < (extendedPxCount + 1))
	{
		throw std::exception("Изображение слишком мало для интерполяции выбранным способом");
	}

	widthPx = header.imageWidthPx + extendedPxCount * 2;
	heightPx = header.imageHeightPx + extendedPxCount * 2;
	image = vector<uint8_t>(widthPx * heightPx * 3);

	int originalWidthBytes = header.imageWidthPx * 3;
	int extendedWidthBytes = widthPx * 3;
	int paddingBytesCount = ((originalWidthBytes + 3) & ~3) - originalWidthBytes;

	input.seekg(header.imageOffsetBytes);

	// Загрузка изображения
	for (size_t y = extendedPxCount;
		y < header.imageHeightPx + extendedPxCount;
		y++)
	{
		input.read(
			(char*)&image[extendedPxCount * 3 + extendedWidthBytes * y],
			originalWidthBytes
		);
		input.seekg(paddingBytesCount, std::ios_base::cur);

		MirrorRowEdges(
			extendedWidthBytes * y, 
			header.imageWidthPx, 
			extendedPxCount
		);
	}

	// Отражение первых строк
	for (int y = 0; y < extendedPxCount; y++)
	{
		int mirrorY = 2 * extendedPxCount - 1 - y;

		std::copy(
			&image[mirrorY * extendedWidthBytes],
			&image[(mirrorY + 1) * extendedWidthBytes],
			&image[y * extendedWidthBytes]
		);
	}

	// Отражение последних строк
	for (int y = header.imageHeightPx + extendedPxCount;
		y < header.imageHeightPx + extendedPxCount * 2;
		y++)
	{
		int mirrorY = 2 * (header.imageHeightPx + extendedPxCount) - y - 1;

		std::copy(
			&image[mirrorY * extendedWidthBytes],
			&image[(mirrorY + 1) * extendedWidthBytes],
			&image[y * extendedWidthBytes]
		);
	}
}

int ImageData::GetExtendedPxCount() const noexcept
{
	return extendedPxCount;
}

uint8_t ImageData::operator()(
	int x, int y, 
	uint32_t colorOffset) const
{
	return image[(y * widthPx + x) * 3 + colorOffset];
}
