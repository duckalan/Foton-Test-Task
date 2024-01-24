#pragma once
#include "BmpHeader.h"
#include "ImageData.h"
#include "Point.h"
#include <array>
#include <cmath>
#include <vector>

using std::vector;
using std::array;

inline float Frac(float x) noexcept
{
	return x - int32_t(x);
}

inline array<uint8_t, 3> NearestNeighbor(
	const Point& p1,
	const ImageData& image)
{
	float x = round(p1.x);
	float y = round(p1.y);

	return array<uint8_t, 3>
	{
		image(x, y, 0), image(x, y, 1), image(x, y, 2),
	};
}


// 0 <= x <= 1
inline float Lerp(
	float x,
	float f0,
	float f1)
{
	return (1 - x) * f0 + x * f1;
}

inline array<uint8_t, 3> BiLerp(
	const Point& p1,
	const ImageData& image)
{
	array<uint8_t, 3> result{};
	float x = Frac(p1.x);
	float y = Frac(p1.y);

	for (size_t colorOffset = 0; colorOffset < 3; colorOffset++)
	{
		float l1 = Lerp(
			y,
			image(p1.x, p1.y, colorOffset),
			image(p1.x, ceil(p1.y), colorOffset));

		float l2 = Lerp(
			y,
			image(ceil(p1.x), p1.y, colorOffset),
			image(ceil(p1.x), ceil(p1.y), colorOffset));

		result[colorOffset] = (uint8_t)(
			std::clamp(
				Lerp(x, l1, l2) + 1.f,
				0.f,
				255.f)
			);
	}

	return result;
}

// 0 <= x <= 1
inline float Cubic(
	float x, 
	float f0, 
	float f1, 
	float f2, 
	float f3)
{
	return f1 + 0.5f * x * (f2 - f0 + x * (2.0f * f0 - 5.0f * f1 + 4.0f * f2 - f3 + x * (3.0f * (f1 - f2) + f3 - f0)));
}

inline array<uint8_t, 3> BiCubic(
	const Point& p1, 
	const ImageData& image)
{
	// Координаты точки p.1 в дополненном изображении
	float realX = p1.x + image.GetExtendedPxCount();
	float realY = p1.y + image.GetExtendedPxCount();

	int xm1 = floor(realX - 1);	// x_-1
	int x0 = floor(realX);		// x_0
	int x1 = ceil(realX);		// x_1
	int x2 = ceil(realX + 1);	// x_2

	array<int, 4> yCoefs{
		floor(realY - 1),	// y_-1
		floor(realY),		// y_0
		ceil(realY),		// y_1
		ceil(realY + 1)		// y_2
	};

	array<uint8_t, 3> result{};
	array<float, 4> bCoefs{};
	float x = Frac(p1.x);
	float y = Frac(p1.y);

	for (size_t colorOffset = 0; colorOffset < 3; colorOffset++)
	{
		for (size_t i = 0; i < 4; i++)
		{
			bCoefs[i] = Cubic(
				x,
				image(xm1, yCoefs[i], colorOffset),
				image(x0, yCoefs[i], colorOffset),
				image(x1, yCoefs[i], colorOffset),
				image(x2, yCoefs[i], colorOffset)
			);
		}

		result[colorOffset] = (uint8_t)(
			std::clamp(
				Cubic(y, bCoefs[0], bCoefs[1], bCoefs[2], bCoefs[3]) + 1.f,
				0.f,
				255.f)
			);
	}

	return result;
}


inline array<uint8_t, 3> Lanczos2(
	const Point& p1, 
	const ImageData& image)
{
	return array<uint8_t, 3>{};
}

inline array<uint8_t, 3> Lanczos3(
	const Point& p1,
	const ImageData& image)
{
	return array<uint8_t, 3>{};
}