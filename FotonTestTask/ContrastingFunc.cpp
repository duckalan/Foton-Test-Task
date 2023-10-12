//#include "ContrastingFunc.h"
//
//ContrastingFunc::ContrastingFunc(
//	const std::vector<uint32_t>& histogram,
//	uint32_t histogramSquare, 
//	float minContrastBorder, 
//	float maxContrastBorder)
//{
//	const float minSquare = histogramSquare * minContrastBorder;
//	const float maxSquare = histogramSquare * maxContrastBorder;
//
//	int32_t squareSum = 0;
//	pseudoMin_ = 0;
//	while (squareSum < minSquare && pseudoMin_ < HistogramSize)
//	{
//		squareSum += histogram[pseudoMin_++];
//	}
//
//	squareSum = histogramSquare;
//	pseudoMax_ = HistogramSize - 1;
//	while (squareSum > maxSquare && pseudoMax_ > 0)
//	{
//		squareSum -= histogram[pseudoMax_--];
//	}
//
//	if (pseudoMin_ > pseudoMax_)
//	{
//		std::swap(pseudoMin_, pseudoMax_);
//	}
//
//	if (pseudoMin_ == pseudoMax_)
//	{
//		pseudoMin_--;
//		pseudoMax_++;
//	}
//}
//
//
//inline uint8_t ContrastingFunc::operator()(float colorValue) const noexcept
//{
//	return (uint8_t)(std::clamp(
//		255.f * (colorValue - pseudoMin_) / (pseudoMax_ - pseudoMin_), 
//		0.f, 255.f) + .5f);
//}
