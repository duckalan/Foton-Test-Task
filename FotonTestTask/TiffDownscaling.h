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

const int ChannelCount = 3;
const int BmpBytePerPx = 3;
const int TiffBytePerPx = 6;
const int BmpImageOffsetBytes = 54;

void DownscaleTiffWithAvgScaling(
	path inputFilePath,
	path outputFilePath,
	int n);

RequiredTiffData ReadTiff(std::ifstream& input);

void SumWindowsInRow(
	const vector<uint16_t>& srcRow,
	vector<float>& sumBuffer,
	int n);

void CalculateAvgValuesInSumBuffer(
	vector<float>& sumBuffer,
	int32_t srcWidthPx, int32_t srcLengthPx,
	bool isBottomEdge,
	int n);

void CopyAvgValuesToDestRowBuffer(
	const vector<float>& avgValues, 
	vector<uint8_t>& destRowBuffer);