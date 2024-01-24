#pragma once

#include <limits>
#include <numbers>
#include <corecrt_math.h>
#include "Point.h"

using std::numeric_limits;

class RotationMatrix
{
private:
	float a11_;
	float a12_;
	float a21_;
	float a22_;
	float b1_;
	float b2_;

public:
	RotationMatrix(float angleDeg, float b1, float b2) noexcept;

	void SetB1(float value) noexcept;

	void SetB2(float value) noexcept;

	Point ReverseTransformation(const Point& point) const noexcept;

	Point ReverseTransformation(float x, float y) const noexcept;

	Point operator *(const Point& point) const noexcept;
};

