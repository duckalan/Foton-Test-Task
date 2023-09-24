#pragma once

#include <vector>
#include <filesystem>
#include "RequiredBmpValues.h"

using std::vector;
using std::filesystem::path;

const int BytePerPx = 3;

void DownscaleBmpWithPixelSkipping(
	path inputFilePath,
	path outputFilePath,
	int n);

void DownscaleBmpWithAvgScailing(
	path inputFilePath,
	path outputFilePath,
	int n);

void SumWindowsInRow(
	const vector<unsigned char>& inputRow,
	vector<float>& sumBuffer,
	int32_t inputImageWidthInPixels,
	int n);

void FindAvgValuesInSumBuffer(
	vector<float>& sumBuffer,
	int32_t srcW, int32_t srcH,
	bool isTopEdge,
	int n);

RequiredBmpValues ReadChangeWriteHeaders(
	std::ifstream& input,
	std::ofstream& output,
	int n);