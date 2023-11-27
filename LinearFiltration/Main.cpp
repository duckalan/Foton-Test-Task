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
		int n = 3;
		int m = 3;
		Kernel kX(1, n, vector<float>(n, 1.f / n));
		Kernel kY(m, 1, vector<float>(m, 1.f/ m));

		auto now = high_resolution_clock::now();
		MovingRmse("H:\\ImageTest\\test1.bmp", "H:\\ImageTest\\outputBoxRMSD1.bmp", kX, kY);
		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "BoxFiltering has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}