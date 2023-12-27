#pragma once

#include <cstdint>

const uint32_t BmpHeaderSize = 54;
const uint32_t ImageOffsetBytes = 54;

#pragma pack(push, 1)
struct BmpHeader
{
	// BmpFileHeader
	uint16_t bm;
	uint32_t fileSizeBytes;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t imageOffsetBytes;

	// DibHeader
	uint32_t dibHeaderSizeBytes;
	int32_t imageWidthPx;
	int32_t imageHeightPx;
	uint16_t colorPlanesCount;
	uint16_t bitPerPixel;
	uint32_t compressionMethod;
	uint32_t imageSizeBytes;
	int32_t xPixelPerMetre;
	int32_t yPixelPerMetre;
	uint32_t paletteColorsCount;
	uint32_t importantColorsCount;

	BmpHeader CreateWithNewSize(int32_t newWidthPx, int32_t newHeightPx)
	{
		uint32_t bytePerPx = bitPerPixel / 8;
		uint32_t stride = (newWidthPx * bytePerPx + 3) & ~3;
		uint32_t paddingBytesCount = stride - newWidthPx * bytePerPx;

		uint32_t newImageSizeBytes = newWidthPx * newHeightPx * bytePerPx + paddingBytesCount * newHeightPx;
		return BmpHeader
		{
			bm,
			BmpHeaderSize + newImageSizeBytes,
			reserved1,
			reserved2,
			imageOffsetBytes,

			dibHeaderSizeBytes,
			newWidthPx,
			newHeightPx,
			colorPlanesCount,
			bitPerPixel,
			compressionMethod,
			newImageSizeBytes,
			xPixelPerMetre,
			yPixelPerMetre,
			paletteColorsCount,
			importantColorsCount
		};
	}
};
#pragma pack(pop)