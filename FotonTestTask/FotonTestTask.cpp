#include <iostream>
#include "Bmp24.h"

// ������� bmp � 24 bpp, ��������� � N ���, ��������� � ����� �����������

int main(int argc, char* argv[])
{
	//std::cout
	auto inputFilePath = "D:\\Code\\FotonTestTask\\Lenna.bmp";
	auto outputFilePath = "D:\\Code\\FotonTestTask\\testO.bmp";

	try
	{
		Bmp24 bmp(inputFilePath);
		auto b2 = bmp.ThinImage(3);
		b2.SaveToFile(outputFilePath);
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}


	//bmp.SaveToFile(outputFilePath);
	std::cout << "Done.\n";
}
