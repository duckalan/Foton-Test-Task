#include <iostream>
#include <chrono>

#include "BmpDownscailing.h"

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

		DownscaleBmpWithAvgScailing(
			"C:\\Users\\Duck\\Desktop\\test\\test1.bmp",
			"C:\\Users\\Duck\\Desktop\\test\\test1NewNewOutput.bmp",
			5);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Thinning has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}