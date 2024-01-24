#pragma once

#include "BmpHeader.h"
#include "InterpolationType.h"
#include <fstream>
#include <vector>

using std::vector;

class ImageData
{
private:
	int widthPx;
	int heightPx;

	/// <summary>
	/// Количество пикселей, на которое изображение 
	/// расширяется по краям для интерполяции 
	/// бикубически или методом Ланцоша.
	/// </summary>
	int extendedPxCount;

	vector<uint8_t> image;

	void MirrorRowEdges(
		int rowOffsetBytes, 
		int originalWidthPx, 
		uint32_t pxToMirrorCount
	);

	void ExtendRowEdges(
		int rowOffsetBytes, 
		int originalWidthBytes, 
		uint32_t pxToAddCount
	);

public:
	ImageData(
		std::ifstream& input,
		const BmpHeader& header,
		InterpolationType interpolationType
	);

	int GetExtendedPxCount() const noexcept;

	uint8_t operator()(
		int x, int y, 
		uint32_t colorOffset) const;
};

