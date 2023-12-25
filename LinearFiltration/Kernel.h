#pragma once

#include <vector>
#include <initializer_list>

class Kernel
{
protected:
	std::vector<float> kernel_;

	int height_;
	int width_;
	int verticalRadius_;
	int horizontalRadius_;

	Kernel(int height, int width);

public:
	Kernel(int height, int width, std::initializer_list<float> kernel);
	Kernel(int height, int width, std::vector<float> kernel);

	int Height() const noexcept;

	int Width() const noexcept;

	// Расстояние от центрального элемента до верхнего/нижнего края ядра
	int VerticalRadius() const noexcept;

	// Расстояние от центрального элемента до левого/правого края ядра
	int HorizontalRadius() const noexcept;

	float operator()(int y, int x) const;
};

