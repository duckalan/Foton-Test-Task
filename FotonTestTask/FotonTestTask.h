#pragma once

#include <numeric>
#include <iostream>
#include <fstream>
#include <chrono>
#include "BmpFileHeader.h"
#include "DibHeader.h"
#include "RequiredInfo.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

const int BytePerPx = 3;

void ThinBmp(const char* inputFilePath, const char* outputFilePath, int n);

void ThinBmpWith(const char* inputFilePath, const char* outputFilePath, int n);

RequiredInfo ReadChangeWriteHeaders(std::ifstream& input, std::ofstream& output, int n);