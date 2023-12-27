#pragma once

#include <fstream>
#include <vector>
#include "BmpHeader.h"

using std::vector;

class ImageData
{
private:
	int widthPx;
	int heightPx;
	int paddingBytesCount;
	vector<uint8_t> vec;

public:
	ImageData(std::ifstream& input, const BmpHeader& header);

	uint8_t GetB(int x, int y) const;
	uint8_t GetG(int x, int y) const;
	uint8_t GetR(int x, int y) const;
};

