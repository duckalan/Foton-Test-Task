#pragma once
#pragma pack (push, 2)
#include <cstdint>

struct BmpFileHeader
{
	uint16_t bm;
	uint32_t fileSize;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t imageOffset;
};

#pragma pack(pop)