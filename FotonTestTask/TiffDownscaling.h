#pragma once
#include <stdint.h>
#include <fstream>
#include <vector>
#include <filesystem>

#include "TiffField.h"
#include "RequiredTiffData.h"
#include "DibHeader.h"
#include "BmpFileHeader.h"

using std::vector;
using std::filesystem::path;

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

const int ChannelCount = 3;
const int BmpBytePerPx = 3;
const int TiffBytePerPx = 6;

void DownscaleTiffWithAvgScaling(
	path inputFilePath,
	path outputFilePath,
	int n);

RequiredTiffData ReadTiff(std::ifstream& input);

void SumWindowsInRow(
	const vector<uint16_t>& srcRow,
	vector<float>& sumBuffer,
	int32_t srcWidthPx,
	int n);

void FindAvgValuesInSumBuffer(
	vector<float>& sumBuffer,
	int32_t srcWidthPx, int32_t srcLengthPx,
	bool isBottomEdge,
	int n);