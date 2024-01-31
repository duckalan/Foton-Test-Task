#pragma once
#include "BmpHeader.h"
#include "ImageData.h"
#include "Point.h"
#include <array>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numbers>

using std::numbers::pi_v;
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
	float f1) noexcept
{
	return (1 - x) * f0 + x * f1;
}

inline array<uint8_t, 3> BiLerp(
	const Point& p1,
	const ImageData& image) noexcept
{
	array<uint8_t, 3> result{};
	float x = Frac(p1.x);
	float y = Frac(p1.y);

	for (size_t colorOffset = 0; colorOffset < 3; colorOffset++)
	{
		float l1 = Lerp(
			y,
			image(p1.x, p1.y, colorOffset),
			image(p1.x, ceil(p1.y), colorOffset)
		);

		float l2 = Lerp(
			y,
			image(ceil(p1.x), p1.y, colorOffset),
			image(ceil(p1.x), ceil(p1.y), colorOffset)
		);

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
	float f3) noexcept
{
	return f1 + 0.5f * x * (f2 - f0 + x * (2.0f * f0 - 5.0f * f1 + 4.0f * f2 - f3 + x * (3.0f * (f1 - f2) + f3 - f0)));
}

inline array<uint8_t, 3> BiCubic(
	const Point& p1,
	const ImageData& image) noexcept
{
	// Координаты точки p.1 в дополненном изображении
	float realX = p1.x + image.GetExtendedPxCount();
	float realY = p1.y + image.GetExtendedPxCount();

	int xm1 = floor(realX - 1);	// x_-1
	int x0 = floor(realX);		// x_0
	int x1 = ceil(realX);		// x_1
	int x2 = ceil(realX + 1);	// x_2

	array<int, 4> yCoords{
		floor(realY - 1),	// y_-1
		floor(realY),		// y_0
		ceil(realY),		// y_1
		ceil(realY + 1)		// y_2
	};

	array<uint8_t, 3> result{};
	array<float, 4> bCoefs{};
	float fracX = Frac(p1.x);
	float fracY = Frac(p1.y);

	for (size_t colorOffset = 0; colorOffset < 3; colorOffset++)
	{
		for (size_t i = 0; i < 4; i++)
		{
			bCoefs[i] = Cubic(
				fracX,
				image(xm1, yCoords[i], colorOffset),
				image(x0, yCoords[i], colorOffset),
				image(x1, yCoords[i], colorOffset),
				image(x2, yCoords[i], colorOffset)
			);
		}

		result[colorOffset] = (uint8_t)(
			std::clamp(
				Cubic(
					fracY,
					bCoefs[0],
					bCoefs[1],
					bCoefs[2],
					bCoefs[3]
				) + 1.f,
				0.f,
				255.f)
			);
	}

	return result;
}


inline float Sinc(float x) noexcept
{
	return sin(x * pi_v<float>) / (x * pi_v<float>);
}

inline float LanczosKernel(float x, int a) noexcept
{
	if (abs(x) < 1e-5)
	{
		return 1.f;
	}

	if (x >= -a && x <= a)
	{
		return Sinc(x) * Sinc(x / a);
	}

	return 0.f;
}

// Выход за границы изображения при интерполяции при повороте
// на градус, кратный pi/2
inline array<uint8_t, 3> LanczosInterpolation(
	const Point& p1,
	int a,
	const ImageData& image
) noexcept
{
	array<uint8_t, 3> result{};
	vector<float> convolutedRows(a * 2);
	int startIndexX = floor(p1.x) - a + 1;
	int endIndexX = floor(p1.x) + a;
	int startIndexY = floor(p1.y) - a + 1;
	int endIndexY = floor(p1.y) + a;

	// Заранее вычисление весов ядра по X и их суммы
	// для нормировки 
	float weightSumX = 0;
	vector<float> weightsX(a * 2);
	for (size_t x = startIndexX; x <= endIndexX; x++)
	{
		float weightX = LanczosKernel(p1.x - x, a);
		weightSumX += weightX;
		weightsX[x - startIndexX] = weightX;
	}

	// То же самое по Y
	float weightSumY = 0;
	vector<float> weightsY(a * 2);
	for (size_t y = startIndexY; y <= endIndexY; y++)
	{
		float weightY = LanczosKernel(p1.y - y, a);
		weightSumY += weightY;
		weightsY[y - startIndexY] = weightY;
	}

	for (size_t colorOffset = 0; colorOffset < 3; colorOffset++)
	{
		// Свёртка по X
		for (size_t y = startIndexY; y <= endIndexY; y++)
		{
			for (size_t x = startIndexX; x <= endIndexX; x++)
			{
				convolutedRows[y - startIndexY] +=
					image(x, y, colorOffset) * weightsX[x - startIndexX];
			}
			convolutedRows[y - startIndexY] /= weightSumX;
		}

		// Свёртка по Y
		float convolutionResult = 0.f;
		for (size_t y = startIndexY; y <= endIndexY; y++)
		{
			convolutionResult +=
				convolutedRows[y - startIndexY] * weightsY[y - startIndexY];
		}

		result[colorOffset] = (uint8_t)std::clamp(
			convolutionResult / weightSumY + 1.f,
			0.f,
			255.f
		);

		std::fill(convolutedRows.begin(), convolutedRows.end(), 0.f);
	}

	return result;
}

inline array<uint8_t, 3> Lanczos2(
	const Point& p1,
	const ImageData& image) noexcept
{
	Point realP1(
		p1.x + image.GetExtendedPxCount(),
		p1.y + image.GetExtendedPxCount()
	);

	return LanczosInterpolation(realP1, 2, image);
}

inline array<uint8_t, 3> Lanczos3(
	const Point& p1,
	const ImageData& image) noexcept
{
	Point realP1(
		p1.x + image.GetExtendedPxCount(),
		p1.y + image.GetExtendedPxCount()
	);

	return LanczosInterpolation(realP1, 3, image);
}