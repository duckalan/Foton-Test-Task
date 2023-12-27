#pragma once
#include <cmath>
#include <vector>
#include <array>
#include "Point.h"
#include "BmpHeader.h"
#include "ImageData.h"

using std::vector;
using std::array;

inline array<uint8_t, 3> BilinearInterpolation(
	Point p1,
	const ImageData& image)
{
	float x = p1.x;
	float y = p1.y;
	float x1 = floor(p1.x);
	float x2 = ceil(p1.x);
	float y1 = floor(p1.y);
	float y2 = ceil(p1.y);

	float b11 = image.GetB(x1, y1);
	float b12 = image.GetB(x1, y2);
	float b21 = image.GetB(x2, y1);
	float b22 = image.GetB(x2, y2);

	float g11 = image.GetG(x1, y1);
	float g12 = image.GetG(x1, y2);
	float g21 = image.GetG(x2, y1);
	float g22 = image.GetG(x2, y2);

	float r11 = image.GetR(x1, y1);
	float r12 = image.GetR(x1, y2);
	float r21 = image.GetR(x2, y1);
	float r22 = image.GetR(x2, y2);

	float firstCoef = 1 / ((x2 - x1) * (y2 - y1));

	float b = firstCoef * (b11 * (x2 - x) * (y2 - y) + b21 * (x - x1) * (y2 - y) + b12 * (x2 - x) * (y - y1) + b22 * (x - x1) * (y - y1));
	float g = firstCoef * (g11 * (x2 - x) * (y2 - y) + g21 * (x - x1) * (y2 - y) + g12 * (x2 - x) * (y - y1) + g22 * (x - x1) * (y - y1));
	float r = firstCoef * (r11 * (x2 - x) * (y2 - y) + r21 * (x - x1) * (y2 - y) + r12 * (x2 - x) * (y - y1) + r22 * (x - x1) * (y - y1));

	return array<uint8_t, 3>
	{
		(uint8_t)(b + 1),
		(uint8_t)(g + 1),
		(uint8_t)(r + 1),
	};
}