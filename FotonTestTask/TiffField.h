#pragma once

#include <stdint.h>

const uint32_t littleEndianTiffIdentifier = 0x2a'4949u;

const uint16_t imageWithoutCompression = 1;

const uint16_t imageWidthTag = 0x100;
const uint16_t imageLengthTag = 0x101;
const uint16_t bitsPerSampleTag = 0x102;
const uint16_t compressionTag = 0x103;
const uint16_t stripOffsetsTag = 0x111;
const uint16_t rowsPerStripTag = 0x116;

// pack добавить по 1
struct TiffField
{
	/// <summary>
	/// Tag identifier
	/// </summary>
	uint16_t tag;

	/// <summary>
	/// Value type
	/// </summary>
	uint16_t type;

	/// <summary>
	/// Number of values
	/// </summary>
	uint32_t count;

	/// <summary>
	/// Value if count == 1 or value offset
	/// </summary>
	uint32_t valueOffset;
};