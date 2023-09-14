#pragma once

#include <cstdint>

struct DibHeader 
{
	uint32_t headerSize;

	/// <summary>
	/// The image width in pixels.
	/// </summary>
	int32_t imageWidth;
	
	/// <summary>
	/// The image height in pixels.
	/// </summary>
	int32_t imageHeight;

	uint16_t colorPlanesCount;
	uint16_t bitPerPixel;
	uint32_t compressionMethod;
	uint32_t imageSize;
	int32_t xPixelPerMetre;
	int32_t yPixelPerMetre;
	uint32_t paletteColorsCount;
	uint32_t importantColorsCount;
};