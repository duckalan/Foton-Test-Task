#include <iostream>
#include "Bmp24.h"

// Read bmp with color depth 24 bits per pixel, 
// thin it by N times, save thinned image in new bmp

int main(int argc, char* argv[])
{
	const int inputFilePathIndex = 1;
	const int outputFilePathIndex = 2;
	const int nIndex = 3;

	bool isThinningSuccessful = true;

	try
	{
		Bmp24 inputBmp(argv[inputFilePathIndex]);
		auto thinnedBmp = inputBmp.ThinImage(atoi(argv[nIndex]));
		thinnedBmp.SaveToFile(argv[outputFilePathIndex]);
	}
	catch (const std::invalid_argument& e)
	{
		isThinningSuccessful = false;
		std::cout << e.what() << '\n';
	}

	if (isThinningSuccessful)
	{
		std::cout << "Thinning has completed.\n";
	}
}