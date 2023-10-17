#pragma once

#include <cstdint>

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
};
#pragma pack(pop)