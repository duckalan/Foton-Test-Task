#pragma once

#include <iostream>
#include <fstream>
#include <chrono>
#include "BmpFileHeader.h"
#include "DibHeader.h"
#include "ReadedInfo.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

const int bytePerPx = 3;

void ThinBmp(const char* inputFilePath, const char* outputFilePath, int n);

ReadedInfo ReadChangeWriteHeaders(std::ifstream& input, std::ofstream& output, int n);