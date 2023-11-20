#include "Kernel.h"

Kernel::Kernel(int height, int width)
{
	height_ = height;
	width_ = width;

	verticalRadius_ = height / 2;
	horizontalRadius_ = width / 2;

	kernel_ = std::vector<float>(height * width);
}

Kernel::Kernel(int height, int width, std::initializer_list<float> kernel)
{
	if (kernel.size() != height * width)
	{
		throw std::exception("Переданные высота и ширина не равны размеру переданного вектора");
	}

	kernel_ = std::move(kernel);
	height_ = height;
	width_ = width;

	verticalRadius_ = height / 2;
	horizontalRadius_ = width / 2;
}

Kernel::Kernel(int height, int width, std::vector<float> kernel)
{
	if (kernel.size() != height * width)
	{
		throw std::exception("Переданные высота и ширина не равны размеру переданного вектора");
	}

	kernel_ = std::move(kernel);
	height_ = height;
	width_ = width;

	verticalRadius_ = height / 2;
	horizontalRadius_ = width / 2;
}

int Kernel::Height() const noexcept
{
	return height_;
}

int Kernel::Width() const noexcept
{
	return width_;
}

int Kernel::VerticalRadius() const noexcept
{
	return verticalRadius_;
}

int Kernel::HorizontalRadius() const noexcept
{
	return horizontalRadius_;
}

float Kernel::operator()(int y, int x) const
{
	if (x >= width_)
	{
		throw std::exception("Выход пределы ширины ядра");
	}

	return kernel_[(size_t)y * width_ + x];
}
