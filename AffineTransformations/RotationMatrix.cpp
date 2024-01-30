#include "RotationMatrix.h"

RotationMatrix::RotationMatrix(float angleDeg, float b1, float b2) noexcept
{
	float angleRad = std::numbers::pi_v<float> * angleDeg / 180.f;
	a11_ = cos(angleRad);
	a21_ = sin(angleRad);
	a12_ = -a21_;
	a22_ = a11_;

	b1_ = b1;
	b2_ = b2;
}

void RotationMatrix::SetB1(float value) noexcept
{
	b1_ = value;
}

void RotationMatrix::SetB2(float value) noexcept
{
	b2_ = value;
}

Point RotationMatrix::ReverseTransformation(const Point& point) const noexcept
{
	return Point(
		a11_ * (point.x - b1_) - a12_ * (point.y - b2_), // x1
		-a21_ * (point.x - b1_) + a22_ * (point.y - b2_) // y1
	);
}

Point RotationMatrix::ReverseTransformation(float x, float y) const noexcept
{
	return Point(
		a11_ * (x - b1_) - a12_ * (y - b2_), // x1
		-a21_ * (x - b1_) + a22_ * (y - b2_) // y1
	);
}

Point RotationMatrix::operator*(const Point& point) const noexcept
{
	return Point(
		a11_ * point.x + a12_ * point.y + b1_, // x2
		a21_ * point.x + a22_ * point.y + b2_  // y2
	);
}
