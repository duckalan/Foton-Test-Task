#include <iostream>
#include <chrono>

#include "TiffDownscaling.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main(int argc, char* argv[])
{
	const int inputFilePathIndex = 1;
	const int outputFilePathIndex = 2;
	const int nIndex = 3;
	
	try
	{
		auto now = high_resolution_clock::now();

		DownscaleTiffWithAvgScaling(
			"H:\\ImageTest\\0041_0102_01567_1_01497_03_S.tiff",
			"H:\\ImageTest\\output.bmp",
			8);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Thinning has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}