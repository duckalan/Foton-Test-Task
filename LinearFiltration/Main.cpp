#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <numeric>

#include "Kernel.h"
#include "BmpHeader.h"

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

const int BytePerPx = 3;

void MirrorEdgePixelsInRow(
	vector<uint8_t>& expandedRows,
	int rowIndex,
	int imageWidthPx,
	const Kernel& kernel)
{
	int expandedWidthBytes = expandedRows.size() / kernel.Height();
	int currentRowIndexBytes = expandedWidthBytes * rowIndex;

	// Ось отражения - нулевой пиксель, у расширенной строки
	// его индекс = kernel.HorizontalRadius
	// Левый край
	for (int x = 0; x < kernel.HorizontalRadius(); x++)
	{
		int mirrorX = kernel.Height() - 2 - x;

		expandedRows[currentRowIndexBytes + x * BytePerPx] =
			expandedRows[currentRowIndexBytes + mirrorX * BytePerPx];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 1] =
			expandedRows[currentRowIndexBytes + mirrorX * BytePerPx + 1];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 2] =
			expandedRows[currentRowIndexBytes + mirrorX * BytePerPx + 2];
	}

	// Правый край
	for (int x = imageWidthPx + kernel.HorizontalRadius();
		x < imageWidthPx + kernel.HorizontalRadius() * 2;
		x++)
	{
		int mirrorX = 2 * (imageWidthPx + kernel.HorizontalRadius()) - x - 1;

		expandedRows[currentRowIndexBytes + x * BytePerPx] =
			expandedRows[currentRowIndexBytes + mirrorX * BytePerPx];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 1] =
			expandedRows[currentRowIndexBytes + mirrorX * BytePerPx + 1];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 2] =
			expandedRows[currentRowIndexBytes + mirrorX * BytePerPx + 2];
	}
}

void FilterImage(
	std::filesystem::path srcPath,
	std::filesystem::path destPath,
	const Kernel& kernel)
{
	std::ifstream src(srcPath, std::ios::binary);
	if (!src.is_open())
	{
		std::string errorMessage = "Can't find or open file with input path: ";
		throw std::invalid_argument(errorMessage + srcPath.string());
	}
	std::ofstream dest(destPath, std::ios::binary);
	if (!dest.is_open())
	{
		std::string errorMessage = "Can't save file with output path: ";
		throw std::invalid_argument(errorMessage + destPath.string());
	}

	BmpHeader header{};
	src.read((char*)&header, sizeof(header));

	// Проверка на возможность отражения
	if (kernel.HorizontalRadius() > header.imageWidthPx ||
		kernel.VerticalRadius() > header.imageHeightPx)
	{
		throw std::invalid_argument("Изображение слишком мало");
	}

	dest.write((char*)&header, sizeof(header));

	int imageWidthBytes = header.imageWidthPx * BytePerPx;
	int rowStrideBytes = (imageWidthBytes + 3) & ~3;
	int paddingBytesCount = rowStrideBytes - imageWidthBytes;

	// Длина расширенной отражёнными краевыми пикселями строки
	int expandedWidthBytes = imageWidthBytes + kernel.HorizontalRadius() * 2 * BytePerPx;

	vector<uint8_t> srcRowsBufferEx(expandedWidthBytes * kernel.Height());
	vector<uint8_t> destRowBuffer(rowStrideBytes);

	// Формирование первоначального буфера строк с отражением
	// 1 0 0 1 2
	for (int bufferY = 0; bufferY < kernel.Height(); bufferY++)
	{
		int imageY;

		// Отражение
		if (bufferY < kernel.VerticalRadius())
		{
			imageY = kernel.VerticalRadius() - 1 - bufferY;
		}
		else
		{
			imageY = bufferY - kernel.VerticalRadius();
		}

		src.seekg(header.imageOffsetBytes + imageY * (imageWidthBytes + paddingBytesCount));

		src.read((char*)&srcRowsBufferEx[
			kernel.HorizontalRadius() * BytePerPx + expandedWidthBytes * bufferY],
			imageWidthBytes);

		MirrorEdgePixelsInRow(srcRowsBufferEx, bufferY, header.imageWidthPx, kernel);
	}
	src.seekg(header.imageOffsetBytes);

	// Индекс первой строки в буфере
	int firstRowIndex = 0;

	for (int y = kernel.VerticalRadius();
		y < header.imageHeightPx + kernel.VerticalRadius();
		y++)
	{
		int xOffsetPx = 0;

		for (int x = kernel.HorizontalRadius();
			x < header.imageWidthPx + kernel.HorizontalRadius();
			x++)
		{
			float sumB = 0;
			float sumG = 0;
			float sumR = 0;

			// Свёртка
			for (int i = 0; i < kernel.Height(); i++)
			{
				int currentRowIndex = (i + firstRowIndex) % kernel.Height();

				for (int j = xOffsetPx; j < xOffsetPx + kernel.Width(); j++)
				{
					// kernel(currentRowIndex, j - xOffsetPx)
					sumB += kernel(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx];

					sumG += kernel(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx + 1];

					sumR += kernel(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx + 2];
				}
			}

			destRowBuffer[(x - kernel.HorizontalRadius()) * BytePerPx]
				= (uint8_t)std::clamp(sumB + 0.5f, 0.f, 255.f);

			destRowBuffer[(x - kernel.HorizontalRadius()) * BytePerPx + 1]
				= (uint8_t)std::clamp(sumG + 0.5f, 0.f, 255.f);

			destRowBuffer[(x - kernel.HorizontalRadius()) * BytePerPx + 2]
				= (uint8_t)std::clamp(sumR + 0.5f, 0.f, 255.f);

			xOffsetPx++;
		}

		// Отражение 
		int imageY = y;
		if (y >= header.imageHeightPx - 1)
		{
			imageY = 2 * header.imageHeightPx - y - 1;
			src.seekg(header.imageOffsetBytes + (rowStrideBytes * imageY));
		}

		// Запись следующей строки на место первой
		src.read(
			(char*)&srcRowsBufferEx[kernel.HorizontalRadius() * BytePerPx
			+ expandedWidthBytes * firstRowIndex],
			imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);
		MirrorEdgePixelsInRow(srcRowsBufferEx, firstRowIndex, header.imageWidthPx, kernel);

		// Вторая становится первой и т.д.
		firstRowIndex = (firstRowIndex + 1) % kernel.Height();

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}

int main()
{
	const int n = 3;
	Kernel kernel(n, n, {
		0, -1, 0,
		-1, 4, -1,
		0, -1, 0
		});

	try
	{
		auto now = high_resolution_clock::now();

		FilterImage(
			"H:\\ImageTest\\test2.bmp",
			"H:\\ImageTest\\filterOutput.bmp",
			kernel);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Filtering has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}