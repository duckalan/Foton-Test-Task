#pragma once
#include <cstdint>

struct RequiredBmpValues 
{
	int32_t inputHeight;
	int32_t inputWidth;
	int64_t inputStride;
	int32_t inputPaddingBytesCount;
	int32_t outputWidth;
	int64_t outputStride;
};