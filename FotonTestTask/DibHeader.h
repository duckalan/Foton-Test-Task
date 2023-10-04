#pragma once

#include <cstdint>
#include "RequiredTiffData.h"

struct DibHeader 
{
	DibHeader();

	DibHeader(const RequiredTiffData& tiffData);

	uint32_t headerSize = 40;

	/// <summary>
	/// The image width in pixels.
	/// </summary>
	int32_t imageWidth = 0;
	
	/// <summary>
	/// The image height in pixels.
	/// </summary>
	int32_t imageHeight = 0;

	uint16_t colorPlanesCount = 1;
	uint16_t bitPerPixel = 24;
	uint32_t compressionMethod = 0;
	uint32_t imageSize = 0;
	int32_t xPixelPerMetre = 0;
	int32_t yPixelPerMetre = 0;
	uint32_t paletteColorsCount = 0;
	uint32_t importantColorsCount = 0;
};