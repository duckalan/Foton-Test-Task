#include <iostream>
#include <chrono>
#include <vector>

#include "Kernel.h"
#include "FilterFunctions.h"

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main()
{
	const int m = 1;
	const int n = 1;
	Kernel kernel(m, n, vector<float>(m * n, 1.f / (m * n)));

	try
	{
		auto now = high_resolution_clock::now();

		FilterImage(
			"H:\\ImageTest\\test2.bmp",
			"H:\\ImageTest\\output.bmp",
			kernel
		);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Filtering has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}