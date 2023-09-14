#include <iostream>
#include "Bmp24.h"

// Считать bmp с 24 bpp, прорежить в N раз, сохранить в новое изображение

int main(int argc, char* argv[])
{
	//std::cout
	auto inputFilePath = "D:\\Code\\FotonTestTask\\test2.bmp";
	auto outputFilePath = "D:\\Code\\FotonTestTask\\testO.bmp";

	try
	{
		Bmp24 bmp(inputFilePath);
		auto b2 = bmp.ThinImage(5);
		b2.SaveToFile(outputFilePath);
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}


	//bmp.SaveToFile(outputFilePath);
	std::cout << "Done.\n";
}
