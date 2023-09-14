#pragma once

#include <fstream>
#include <vector>
#include "BmpFileHeader.h"
#include "DibHeader.h"
#include "Bgr24.h"

class Bmp24
{
private:
	BmpFileHeader fileHeader_ = {};
	DibHeader dibHeader_ = {};

	/// <summary>
	/// Flipped over bgr pixels without padding bytes
	/// </summary>
	std::vector<Bgr24> imageData_;

	Bmp24(const BmpFileHeader& fileHeader,
		const DibHeader& dibHeader,
		const std::vector<Bgr24>& imageData);

public:
	Bmp24(const char* filePath);

	Bmp24 ThinImage(int n) const;

	void SaveToFile(const char* filePath) const;
};

