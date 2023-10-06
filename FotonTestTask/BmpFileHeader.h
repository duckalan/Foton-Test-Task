#pragma once
#pragma pack (push, 1)
#include <cstdint>

struct BmpFileHeader
{
	uint16_t bm = 0x4d42;
	uint32_t fileSize = 0;
	uint16_t reserved1 = 0;
	uint16_t reserved2 = 0;
	uint32_t imageOffset = 54;
};

#pragma pack(pop)