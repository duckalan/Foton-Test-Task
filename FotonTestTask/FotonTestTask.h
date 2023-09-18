#pragma once

#include <iostream>
#include <fstream>
#include <chrono>
#include "Bgr24.h"
#include "BmpFileHeader.h"
#include "DibHeader.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

void ThinBmpImage(const char* inputFilePath, const char* outputFilePath, int n);