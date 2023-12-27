#include <filesystem>
#include <iostream>
#include <ios>
#include "InterpolationFuncs.h"
#include "RotationMatrix.h"

using std::filesystem::path;
using std::ifstream;
using std::ofstream;

Point FindBCoeffs(const array<Point, 4>& cornerPoints, const RotationMatrix& mat)
{
	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	for (const Point& p : cornerPoints)
	{
		Point rotatedP = mat * p;
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

Point FindRotatedImageSize(const array<Point, 4>& cornerPoints, const RotationMatrix& mat)
{
	float maxX = -std::numeric_limits<float>::max();
	float maxY = -std::numeric_limits<float>::max();
	for (const Point& p : cornerPoints)
	{
		Point rotatedP = mat * p;
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

void RotateImage(
	const path& inputPath, 
	const path& outputPath, 
	float angleDeg)
{
	std::ifstream input(inputPath, std::ios_base::binary);
	std::ofstream output(outputPath, std::ios_base::binary);
	if (!input.is_open() || !output.is_open())
	{
		throw std::ios_base::failure("Can't open input or output files");
	}

	BmpHeader header{};
	input.read((char*)&header, BmpHeaderSize);
	
	RotationMatrix rotMatrix(angleDeg, 0.f, 0.f);
	array<Point, 4> cornerPoints{
		Point(0.f, 0.f),
		Point(header.imageWidthPx - 1, 0.f), 
		Point(0.f, header.imageHeightPx - 1),
		Point(header.imageWidthPx - 1, header.imageHeightPx - 1)
	};
	
	// Нахождение коэффициентов b1, b2
	Point minPoint = FindBCoeffs(cornerPoints, rotMatrix);
	rotMatrix.SetB1(-minPoint.x);
	rotMatrix.SetB2(-minPoint.y);

	// Нахождение размеров перевёрнутого изображения
	Point maxPoint = FindRotatedImageSize(cornerPoints, rotMatrix);

	int newWidthPx = ceil(maxPoint.x + 1);
	int newHeightPx = ceil(maxPoint.y + 1);

	BmpHeader newHeader = header.CreateWithNewSize(newWidthPx, newHeightPx);
	output.write((char*)&newHeader, BmpHeaderSize);

	ImageData inputImage(input, header);

	uint32_t outputRowStride = (newHeader.imageWidthPx * 3 + 3) & ~3;
	vector<uint8_t> outputRow(outputRowStride);

	// Заполнение и построчная запись перемещённого изображения
	for (size_t y2 = 0; y2 < newHeader.imageHeightPx; y2++)
	{
		for (size_t x2 = 0; x2 < newHeader.imageWidthPx; x2++)
		{
			Point p1 = rotMatrix.ReverseTransformation(Point(x2, y2));
			// TODO: сделать выбор интерполяции. Для больших фильтров
			// нужно зеркалить или дублировать края

			int x1 = round(p1.x);
			int y1 = round(p1.y);

			if (x1 >= 0 && x1 < header.imageWidthPx &&
				y1 >= 0 && y1 < header.imageHeightPx)
			{
				outputRow[x2 * 3] = inputImage.GetB(x1, y1);
				outputRow[x2 * 3 + 1] = inputImage.GetG(x1, y1);
				outputRow[x2 * 3 + 2] = inputImage.GetR(x1, y1);
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
    auto in = "H:\\ImageTest\\test1.bmp";
    auto out = "H:\\ImageTest\\neares.bmp";
    
	try
	{
		RotateImage(in, out, -45);
		std::cout << "Done\n";
	}
	catch (const std::ios_base::failure& e)
	{
		std::cerr << e.what() << '\n';
	}
}