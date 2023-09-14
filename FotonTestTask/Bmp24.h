#pragma once

#include <fstream>
#include <memory>
#include <vector>
#include "BmpFileHeader.h"
#include "DibHeader.h"
#include "Bgr24.h"

class Bmp24
{
private:
	BmpFileHeader fileHeader_ = {};
	DibHeader dibHeader_ = {};

	// Cleared bgr pixels without padding bytes, saved in down order
	std::vector<Bgr24> imageData_;

	Bmp24(const BmpFileHeader& fileHeader, 
		  const DibHeader&  dibHeader, 
		  const std::vector<Bgr24>& imageData);

public:
	Bmp24(const char* filePath);

	Bmp24 ThinImage(int n) const;

	void SaveToFile(const char* filePath) const;
};

