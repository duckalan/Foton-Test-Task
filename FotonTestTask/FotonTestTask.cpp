#include <iostream>
#include <chrono>

#include "TiffDownscaling.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main()
{
	try
	{
		auto now = high_resolution_clock::now();

		DownscaleTiffWithAvgScaling(
			"H:\\ImageTest\\0041_0102_01567_1_01497_03_S_frag.tiff",
			"H:\\ImageTest\\output1.bmp",
			0.01f, 0.99f,
			10);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Downscaling has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}