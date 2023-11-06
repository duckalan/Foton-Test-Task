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
		int mirrorX = kernel.HorizontalRadius() * 2 - 1 - x;

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

	src.seekg(header.imageOffsetBytes);
	// Формирование первоначального буфера строк с отражением
	// 1 0 0 1 2
	for (int bufferY = kernel.VerticalRadius(); bufferY < kernel.Height(); bufferY++)
	{
		int imageY = bufferY - kernel.VerticalRadius();

		src.read((char*)&srcRowsBufferEx[
			kernel.HorizontalRadius() * BytePerPx + expandedWidthBytes * bufferY],
			imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);

		MirrorEdgePixelsInRow(srcRowsBufferEx, bufferY, header.imageWidthPx, kernel);
	}

	// Отражение
	for (int bufferY = 0; bufferY < kernel.VerticalRadius(); bufferY++)
	{
		int y = 2 * kernel.VerticalRadius() - 1 - bufferY;

		std::copy(srcRowsBufferEx.data() + y * expandedWidthBytes,
			srcRowsBufferEx.data() + (y + 1) * expandedWidthBytes,
			srcRowsBufferEx.data() + bufferY * expandedWidthBytes);
	}

	// Индекс первой строки в буфере
	int firstRowIndex = 0;
	int oldFirstRowIndex = 0;
	int bufIndexToCopy = 0;

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
		//int imageY = y + 1;
		if (y >= header.imageHeightPx - 1)
		{
			if (y == header.imageHeightPx - 1)
			{
				oldFirstRowIndex = firstRowIndex;
			}
			int imageY = 2 * header.imageHeightPx - 2 - y;
			bufIndexToCopy = abs(oldFirstRowIndex - (header.imageHeightPx - imageY)) % kernel.Height();

			std::copy(srcRowsBufferEx.data() + bufIndexToCopy * expandedWidthBytes,
				srcRowsBufferEx.data() + (bufIndexToCopy + 1) * expandedWidthBytes,
				srcRowsBufferEx.data() + firstRowIndex * expandedWidthBytes);
		}
		else
		{
			src.read(
				(char*)&srcRowsBufferEx[kernel.HorizontalRadius() * BytePerPx
				+ expandedWidthBytes * firstRowIndex],
				imageWidthBytes);
			src.seekg(paddingBytesCount, std::ios_base::cur);
		}
		MirrorEdgePixelsInRow(srcRowsBufferEx, firstRowIndex, header.imageWidthPx, kernel);

		// Вторая становится первой и т.д.
		firstRowIndex = (firstRowIndex + 1) % kernel.Height();

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}

void ApplySobelOperator(
	std::filesystem::path srcPath,
	std::filesystem::path destPath)
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

	Kernel gx(3, 3, {
		-1, 0, 1,
		-2, 0, 2,
		-1, 0, 1
		});

	Kernel gy(3, 3, {
		 1,  2,  1,
		 0,  0,  0,
		-1, -2, -1
		});

	// Проверка на возможность отражения
	if (gx.HorizontalRadius() > header.imageWidthPx ||
		gx.VerticalRadius() > header.imageHeightPx)
	{
		throw std::invalid_argument("Изображение слишком мало");
	}

	dest.write((char*)&header, sizeof(header));

	int imageWidthBytes = header.imageWidthPx * BytePerPx;
	int rowStrideBytes = (imageWidthBytes + 3) & ~3;
	int paddingBytesCount = rowStrideBytes - imageWidthBytes;

	// Длина расширенной отражёнными краевыми пикселями строки
	int expandedWidthBytes = imageWidthBytes + gx.HorizontalRadius() * 2 * BytePerPx;

	vector<uint8_t> srcRowsBufferEx(expandedWidthBytes * gx.Height());
	vector<uint8_t> destRowBuffer(rowStrideBytes);

	src.seekg(header.imageOffsetBytes);
	// Формирование первоначального буфера строк с отражением
	// 1 0 0 1 2
	for (int bufferY = gx.VerticalRadius(); bufferY < gx.Height(); bufferY++)
	{
		int imageY = bufferY - gx.VerticalRadius();

		src.read((char*)&srcRowsBufferEx[
			gx.HorizontalRadius() * BytePerPx + expandedWidthBytes * bufferY],
			imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);

		MirrorEdgePixelsInRow(srcRowsBufferEx, bufferY, header.imageWidthPx, gx);
	}

	// Отражение
	for (int bufferY = 0; bufferY < gx.VerticalRadius(); bufferY++)
	{
		int y = 2 * gx.VerticalRadius() - 1 - bufferY;

		std::copy(srcRowsBufferEx.data() + y * expandedWidthBytes,
			srcRowsBufferEx.data() + (y + 1) * expandedWidthBytes,
			srcRowsBufferEx.data() + bufferY * expandedWidthBytes);
	}

	// Индекс первой строки в буфере
	int firstRowIndex = 0;
	int oldFirstRowIndex = 0;
	int bufIndexToCopy = 0;

	for (int y = gx.VerticalRadius();
		y < header.imageHeightPx + gx.VerticalRadius();
		y++)
	{
		int xOffsetPx = 0;

		for (int x = gx.HorizontalRadius();
			x < header.imageWidthPx + gx.HorizontalRadius();
			x++)
		{
			float xSumB = 0;
			float xSumG = 0;
			float xSumR = 0;

			float ySumB = 0;
			float ySumG = 0;
			float ySumR = 0;

			// Свёртка
			for (int i = 0; i < gx.Height(); i++)
			{
				int currentRowIndex = (i + firstRowIndex) % gx.Height();

				for (int j = xOffsetPx; j < xOffsetPx + gx.Width(); j++)
				{
					xSumB += gx(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx];

					xSumG += gx(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx + 1];

					xSumR += gx(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx + 2];

					ySumB += gy(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx];

					ySumG += gy(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx + 1];

					ySumR += gy(i, j - xOffsetPx) *
						srcRowsBufferEx[currentRowIndex * expandedWidthBytes + j * BytePerPx + 2];
				}
			}

			destRowBuffer[(x - gx.HorizontalRadius()) * BytePerPx]
				= (uint8_t)std::clamp(sqrtf(xSumB * xSumB + ySumB * ySumB) + 0.5f, 0.f, 255.f);

			destRowBuffer[(x - gx.HorizontalRadius()) * BytePerPx + 1]
				= (uint8_t)std::clamp(sqrtf(xSumG * xSumG + ySumG * ySumG) + 0.5f, 0.f, 255.f);

			destRowBuffer[(x - gx.HorizontalRadius()) * BytePerPx + 2]
				= (uint8_t)std::clamp(sqrtf(xSumR * xSumR + ySumR * ySumR) + 0.5f, 0.f, 255.f);

			xOffsetPx++;
		}

		// Отражение 
		//int imageY = y + 1;
		if (y >= header.imageHeightPx - 1)
		{
			if (y == header.imageHeightPx - 1)
			{
				oldFirstRowIndex = firstRowIndex;
			}
			int imageY = 2 * header.imageHeightPx - 2 - y;
			bufIndexToCopy = abs(oldFirstRowIndex - (header.imageHeightPx - imageY)) % gx.Height();

			std::copy(srcRowsBufferEx.data() + bufIndexToCopy * expandedWidthBytes,
				srcRowsBufferEx.data() + (bufIndexToCopy + 1) * expandedWidthBytes,
				srcRowsBufferEx.data() + firstRowIndex * expandedWidthBytes);
		}
		else
		{
			src.read(
				(char*)&srcRowsBufferEx[gx.HorizontalRadius() * BytePerPx
				+ expandedWidthBytes * firstRowIndex],
				imageWidthBytes);
			src.seekg(paddingBytesCount, std::ios_base::cur);
		}
		MirrorEdgePixelsInRow(srcRowsBufferEx, firstRowIndex, header.imageWidthPx, gx);

		// Вторая становится первой и т.д.
		firstRowIndex = (firstRowIndex + 1) % gx.Height();

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}

int main()
{
	const int m = 5;
	const int n = 5;
	Kernel kernel(m, n, vector<float>(m * n, 1.f / (m * n)));

	try
	{
		auto now = high_resolution_clock::now();

		FilterImage(
			"H:\\ImageTest\\test1.bmp",
			"H:\\ImageTest\\test1O.bmp",
			kernel);

		ApplySobelOperator(
			"H:\\ImageTest\\test1.bmp",
			"H:\\ImageTest\\test1Sobely.bmp"
		);

		auto resultTime = duration_cast<milliseconds>(high_resolution_clock::now() - now);
		std::cout << "Filtering has been completed in " << resultTime.count() << " ms.\n";
	}
	catch (const std::invalid_argument& e)
	{
		std::cout << e.what() << '\n';
	}
}