#pragma once

#include <stdint.h>

const uint32_t LittleEndianTiffIdentifier = 0x2a'4949u;
const uint16_t ImageWithoutCompression = 1;

const uint16_t ImageWidthTag = 0x100;
const uint16_t ImageLengthTag = 0x101;
const uint16_t BitsPerSampleTag = 0x102;
const uint16_t CompressionTag = 0x103;
const uint16_t StripOffsetsTag = 0x111;
const uint16_t RowsPerStripTag = 0x116;

#pragma pack(push, 1)
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
#pragma pack(pop)