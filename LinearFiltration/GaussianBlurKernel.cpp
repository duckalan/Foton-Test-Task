#include "GaussianBlurKernel.h"

GaussianBlurKernel::GaussianBlurKernel(int height, int width)
	: Kernel(height, width)
{
	float sigmaY = verticalRadius_ / 3.f;
	float sigmaX = horizontalRadius_ / 3.f;

	float sigY = height == 1 ? 1 : 2 * sigmaY * sigmaY;
	float sigX = width == 1 ? 1 : 2 * sigmaX * sigmaX;

	float sumOfElements = 0;

	// Вычисление коэффициентов
	for (int y = 0; y < height_; y++)
	{
		for (int x = 0; x < width_; x++)
		{
			float yValue = y - verticalRadius_;
			float xValue = x - horizontalRadius_;

			kernel_[y * width_ + x] =
				exp(-(xValue * xValue) / sigX) * exp(-(yValue * yValue) / sigY);

			sumOfElements += kernel_[y * width_ + x];
		}
	}

	// Нормировка
	for (int y = 0; y < height_; y++)
	{
		for (int x = 0; x < width_; x++)
		{
			kernel_[y* width + x] /= sumOfElements;
		}
	}
}
