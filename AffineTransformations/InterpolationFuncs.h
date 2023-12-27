#pragma once
#include <cmath>
#include <vector>
#include <array>
#include "Point.h"
#include "BmpHeader.h"
#include "ImageData.h"

using std::vector;
using std::array;

inline float Frac(float x) noexcept
{
	return x - int32_t(x);
}

inline array<uint8_t, 3> BiLerp(Point p1, const ImageData& image)
{
	float fracX = Frac(p1.x);
	float fracY = Frac(p1.y);
	float invFracX = 1 - fracX;
	float invFracY = 1 - fracY;

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

	float b = b11 * invFracX * invFracY + b21 * fracX * invFracY + b12 * invFracX * fracY + b22 * fracX * fracY;
	float g = g11 * invFracX * invFracY + g21 * fracX * invFracY + g12 * invFracX * fracY + g22 * fracX * fracY;
	float r = r11 * invFracX * invFracY + r21 * fracX * invFracY + r12 * invFracX * fracY + r22 * fracX * fracY;

	return array<uint8_t, 3>
	{
		(uint8_t)(b + 1),
		(uint8_t)(g + 1),
		(uint8_t)(r + 1),
	};
}