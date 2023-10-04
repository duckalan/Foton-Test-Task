#pragma once

#include <vector>

struct RequiredTiffData
{
	uint16_t srcWidthPx;
	uint16_t srcLengthPx;
	uint16_t bitsPerSample;
	std::vector<uint32_t> stripOffsets;
	uint16_t rowsPerStrip;
	uint16_t stripCount;

	uint16_t destWidthPx;
	uint16_t destLengthPx;
	uint32_t destStrideBytes;
};

