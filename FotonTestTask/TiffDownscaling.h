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

// rgb �������
// tiff geotiff. bigtiff
// quick look. 16 bit ���� �������
// ��������� ������ 10 ������� bit
// ��������� �� �� ��������
// ������� ��������� �������������� 10 bit -> 8 bit �������� �� 4
// tiff �������� ��������� ������ ����
// seekg 
// tiff 6.0. ������ � tiff - ����� �������� �����
// little endian �����, �������� ���������
// �������� �� bid endian � tiff
// tag - ������ ������ ����
// ������, ������, ����� �������, ��� �� ����� ����� ���� 1 ������. �������� �� ���������� ��������. ��� ������
// ���� ������ ������ � ������, �� ������ �������� �� ��

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