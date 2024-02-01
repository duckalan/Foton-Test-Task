#include "InterpolationFuncs.h"
#include "InterpolationType.h"
#include "RotationMatrix.h"
#include <filesystem>
#include <ios>
#include <iostream>
#include <functional>

using std::filesystem::path;
using std::string;
using std::ifstream;
using std::ofstream;
using std::function;

Point FindBCoeffs(
	const array<Point, 4>& cornerPoints,
	const RotationMatrix& rotationMatrix)
{
	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	for (const Point& p : cornerPoints)
	{
		Point rotatedP = rotationMatrix * p;
		if (minX > rotatedP.x)
		{
			minX = rotatedP.x;
		}
		if (minY > rotatedP.y)
		{
			minY = rotatedP.y;
		}
	}

	return Point(minX, minY);
}

Point FindRotatedImageSize(
	const array<Point, 4>& cornerPoints,
	const RotationMatrix& rotationMatrix)
{
	float maxX = -std::numeric_limits<float>::max();
	float maxY = -std::numeric_limits<float>::max();
	for (const Point& p : cornerPoints)
	{
		Point rotatedP = rotationMatrix * p;
		if (maxX < rotatedP.x)
		{
			maxX = rotatedP.x;
		}
		if (maxY < rotatedP.y)
		{
			maxY = rotatedP.y;
		}
	}

	return Point(maxX, maxY);
}

function<array<uint8_t, 3>(const Point&, const ImageData&)>
SelectInterpolationFunc(InterpolationType interpolationType)
{
	switch (interpolationType)
	{
	case InterpolationType::NearestNeighbor:
		return &NearestNeighbor;

	case InterpolationType::Bilinear:
		return &BiLerp;

	case InterpolationType::BiCubic:
		return &BiCubic;

	case InterpolationType::Lanczos2:
		return &Lanczos2;

	case InterpolationType::Lanczos3:
		return &Lanczos3;

	default:
		throw std::exception("Wrong enum type");
		break;
	}
}

void RotateImage(
	const path& inputPath,
	const path& outputPath,
	float angleDeg,
	InterpolationType interpolationType)
{
	std::ifstream input(
		inputPath,
		std::ios_base::binary
	);
	std::ofstream output(
		outputPath,
		std::ios_base::binary
	);
	if (!input.is_open() || !output.is_open())
	{
		throw std::ios_base::failure(
			"Can't open input or output files"
		);
	}


	BmpHeader header{};
	input.read((char*)&header, BmpHeaderSize);

	RotationMatrix rotationMatrix(angleDeg, 0.f, 0.f);
	array<Point, 4> cornerPoints{
		Point(0.f, 0.f),
		Point(header.imageWidthPx - 1, 0.f),
		Point(0.f, header.imageHeightPx - 1),
		Point(header.imageWidthPx - 1, header.imageHeightPx - 1)
	};

	// Нахождение коэффициентов b1, b2
	Point minPoint = FindBCoeffs(
		cornerPoints,
		rotationMatrix
	);
	rotationMatrix.SetB1(-minPoint.x);
	rotationMatrix.SetB2(-minPoint.y);

	// Нахождение размеров перевёрнутого изображения
	Point maxPoint = FindRotatedImageSize(
		cornerPoints,
		rotationMatrix
	);

	int newWidthPx = ceil(maxPoint.x + 1);
	int newHeightPx = ceil(maxPoint.y + 1);


	BmpHeader newHeader = header.CreateWithNewSize(
		newWidthPx,
		newHeightPx
	);
	output.write((char*)&newHeader, BmpHeaderSize);

	uint32_t outputRowStride
		= (newHeader.imageWidthPx * 3 + 3) & ~3;

	vector<uint8_t> outputRow(outputRowStride);

	ImageData inputImage(
		input,
		header,
		interpolationType
	);

	function<array<uint8_t, 3>(const Point&, const ImageData&)>
		interpolationFunc = SelectInterpolationFunc(
			interpolationType
		);

	// Заполнение и построчная запись перемещённого изображения
	for (size_t y2 = 0; y2 < newHeightPx; y2++)
	{
		std::cout << std::format(
			"Current progress for Y: {:.2f}%\r", 
			(float)(y2 + 1) / newHeightPx * 100.f
		);

		for (size_t x2 = 0; x2 < newWidthPx; x2++)
		{
			Point p1 = rotationMatrix
				.ReverseTransformation(x2, y2);

			if (p1.x >= 0 &&
				p1.x <= (header.imageWidthPx - 1) &&
				p1.y >= 0 &&
				p1.y <= (header.imageHeightPx - 1))
			{
				array<uint8_t, 3> bgr
					= interpolationFunc(p1, inputImage);

				outputRow[x2 * 3] = bgr[0];
				outputRow[x2 * 3 + 1] = bgr[1];
				outputRow[x2 * 3 + 2] = bgr[2];
			}
			else
			{
				outputRow[x2 * 3] = 0;
				outputRow[x2 * 3 + 1] = 0;
				outputRow[x2 * 3 + 2] = 0;
			}

		}
		output.write((char*)outputRow.data(), outputRowStride);
	}
}

int main()
{
	float angleDeg = 0;
	string rootPath = "C:\\Users\\...\\Desktop\\test images\\";
	string inputImageName = "test2";
	string outputImageName = inputImageName + '_';
	string bmp = ".bmp";
	InterpolationType interpolationType = InterpolationType::Lanczos3;

	switch (interpolationType)
	{
	case InterpolationType::NearestNeighbor:
		outputImageName += "NearestNeighbor";
		break;

	case InterpolationType::Bilinear:
		outputImageName += "Bilinear";
		break;

	case InterpolationType::BiCubic:
		outputImageName += "BiCubic";
		break;

	case InterpolationType::Lanczos2:
		outputImageName += "Lanczos2";
		break;

	case InterpolationType::Lanczos3:
		outputImageName += "Lanczos3";
		break;
	default:
		break;
	}

	outputImageName += std::format("({:.2f}deg)", angleDeg);
	try
	{
		RotateImage(
			rootPath + inputImageName + bmp,
			rootPath + outputImageName + bmp,
			angleDeg,
			interpolationType
		);

		std::cout << "\nDone\n";
	}
	catch (const std::ios_base::failure& e)
	{
		std::cerr << e.what() << '\n';
	}
}