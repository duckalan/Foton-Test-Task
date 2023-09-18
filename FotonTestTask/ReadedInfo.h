#pragma once
#include <cstdint>

struct ReadedInfo {
	int32_t inputHeight;
	int32_t inputWidth;
	int64_t inputStride;
	int32_t inputPaddingBytesCount;
	int64_t outputStride;
};