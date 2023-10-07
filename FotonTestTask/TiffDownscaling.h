#pragma once
#include <stdint.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <functional>
#include <array>

#include "TiffField.h"
#include "RequiredTiffData.h"
#include "DibHeader.h"
#include "BmpFileHeader.h"

using std::array;
using std::function;
using std::vector;
using std::filesystem::path;

const int ChannelCount = 3;
const int BmpBytePerPx = 3;
const int TiffBytePerPx = 6;
const int BmpImageOffsetBytes = 54;
const int HistogramSize = (1 << 16) - 1;

void DownscaleTiffWithAvgScaling(
	path inputFilePath,
	path outputFilePath,
	float minContrastBorder,
	float maxContrastBorder,
	int n);

RequiredTiffData ReadTiff(std::ifstream& input);

void WriteBmpHeaders(const RequiredTiffData& tiffData, std::ofstream& output);

void SumWindowsInRow(
	const vector<uint16_t>& srcRow,
	vector<float>& sumBuffer,
	const array<function<float(float)>, 3>& contrastingFuncs,
	int n);

void CalculateAvgValuesInSumBuffer(
	vector<float>& sumBuffer,
	int32_t srcWidthPx, int32_t srcLengthPx,
	bool isBottomEdge,
	int n);

void CopyAvgValuesToDestRowBuffer(
	const vector<float>& avgValues, 
	vector<uint8_t>& destRowBuffer);


array<function<float(float)>, 3> BuildContrastingFuncs(
	std::ifstream& input,
	const RequiredTiffData& tiffData,
	float minBorder, float maxBorder);

function<float(float)> BuildContrastingFunc(
	const vector<uint32_t> histogram,
	uint32_t histogramSquare,
	float minBorder, float maxBorder);
