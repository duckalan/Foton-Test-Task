#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>

class ContrastingFunc
{
private:
	static const int HistogramSize = (1 << 16) - 1;
	int32_t pseudoMin_;
	int32_t pseudoMax_;

public:
	ContrastingFunc(
		const std::vector<uint32_t>& histogram,
		uint32_t histogramSquare,
		float minContrastBorder,
		float maxContrastBorder) noexcept
	{
		const float minSquare = histogramSquare * minContrastBorder;
		const float maxSquare = histogramSquare * maxContrastBorder;

		int32_t squareSum = 0;
		pseudoMin_ = 0;
		while (squareSum < minSquare && pseudoMin_ < HistogramSize)
		{
			squareSum += histogram[pseudoMin_++];
		}

		squareSum = histogramSquare;
		pseudoMax_ = HistogramSize - 1;
		while (squareSum > maxSquare && pseudoMax_ > 0)
		{
			squareSum -= histogram[pseudoMax_--];
		}

		if (pseudoMin_ > pseudoMax_)
		{
			std::swap(pseudoMin_, pseudoMax_);
		}

		if (pseudoMin_ == pseudoMax_)
		{
			pseudoMin_--;
			pseudoMax_++;
		}
	};

	inline uint8_t operator()(float colorValue) const noexcept 
	{
		return (uint8_t)(std::clamp(
			255.f * (colorValue - pseudoMin_) / (pseudoMax_ - pseudoMin_),
			0.f, 255.f) + .5f);
	}
};

