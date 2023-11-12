#include <iostream>
#include <chrono>
#include <vector>

#include "Kernel.h"
#include "FilterFunctions.h"
#include "LinearlySeparableFiltering.h"

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main()
{
	try
	{
		Kernel k = CreateGaussianBlurKernel(10, 10);
		Kernel kX = CreateGaussianBlurKernel(1, 10);
		Kernel kY = CreateGaussianBlurKernel(10, 1);

		auto now = high_resolution_clock::now();

		FilterImage("H:\\ImageTest\\test1.bmp", "H:\\ImageTest\\output.bmp", kX, kY);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Filtering has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}