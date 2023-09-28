#include <iostream>
#include <chrono>

#include "BmpDownscaling.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main(int argc, char* argv[])
{
	const int inputFilePathIndex = 1;
	const int outputFilePathIndex = 2;
	const int nIndex = 3;

	// rgb порядок
	// tiff geotiff. bigtiff
	// quick look. 16 bit коды яркости
	// заполнено только 10 млашдих bit
	// проделать ту же операцию
	// сделать яркостное преобразование 10 bit -> 8 bit делением на 4
	// tiff хранится нормально сверху вниз
	// seekg 
	// tiff 6.0. Полоса в tiff - набор соседних строк
	// little endian будет, читается нормально
	// проверку на bid endian и tiff
	// tag - массив одного типа
	// высоту, ширину, число каналов, бит на канал может быть 1 числом. Проверка на допустимые значения. без сжатия
	// если строка первая в полосе, то делать смещение на неё
	
	try
	{
		auto now = high_resolution_clock::now();

		DownscaleBmpWithAvgScailing(
			"C:\\Users\\Duck\\Desktop\\test\\test3.bmp",
			"C:\\Users\\Duck\\Desktop\\test\\test2O.bmp",
			5);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Thinning has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}