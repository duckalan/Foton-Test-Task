#include <iostream>
#include <chrono>
#include <vector>

#include "Kernel.h"
#include "FilterFunctions.h"
#include "BoxBlurringFunctions.h"
#include "GaussianBlurKernel.h"

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main()
{
	try
	{
		int n = 100;
		int m = 100;
		//GaussianBlurKernel k(10, 10);
		Kernel k(m, n, vector<float>(n * m, 1.f / (n * m)));
		Kernel kX(1, n, vector<float>(n, 1.f / n));
		Kernel kY(m, 1, vector<float>(m, 1.f/ m));

		auto now = high_resolution_clock::now();

		FilterImage("H:\\ImageTest\\test1.bmp", "H:\\ImageTest\\outputFilter.bmp", k);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Filtering has been completed in " << resultTime.count() << " ms.\n";

		now = high_resolution_clock::now();
		BoxBlur("H:\\ImageTest\\test1.bmp", "H:\\ImageTest\\outputBox.bmp", kX, kY);
		resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "BoxFiltering has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}