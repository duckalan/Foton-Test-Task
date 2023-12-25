#pragma once

#include <fstream>
#include <filesystem>
#include <vector>
#include <numeric>
#include <numbers>

#include "Kernel.h"
#include "BmpHeader.h"

using std::vector;

const int ChannelCount = 3;
const int BytePerPx = 3;
const int BytePerFloatPx = 12;

void MirrorEdgePixelsInRow(
	vector<uint8_t>& expandedRow,
	int imageWidthPx,
	const Kernel& kernelX)
{
	// Левый край
	for (int x = 0; x < kernelX.HorizontalRadius(); x++)
	{
		int mirrorX = kernelX.HorizontalRadius() * 2 - 1 - x;

		expandedRow[x * BytePerPx] = expandedRow[mirrorX * BytePerPx];
		expandedRow[x * BytePerPx + 1] = expandedRow[mirrorX * BytePerPx + 1];
		expandedRow[x * BytePerPx + 2] = expandedRow[mirrorX * BytePerPx + 2];
	}

	// Правый край
	for (int x = imageWidthPx + kernelX.HorizontalRadius();
		x < imageWidthPx + kernelX.HorizontalRadius() * 2;
		x++)
	{
		int mirrorX = 2 * (imageWidthPx + kernelX.HorizontalRadius()) - x - 1;

		expandedRow[x * BytePerPx] = expandedRow[mirrorX * BytePerPx];
		expandedRow[x * BytePerPx + 1] = expandedRow[mirrorX * BytePerPx + 1];
		expandedRow[x * BytePerPx + 2] = expandedRow[mirrorX * BytePerPx + 2];
	}
}

void ConvolutionX(
	const vector<uint8_t>& expandedRow,
	vector<float>& outputRowsBuffer,
	int currentRowOffset,
	const Kernel& kernelX,
	int imageWidthPx)
{
	int xOffsetPx = 0;

	for (int x = kernelX.HorizontalRadius();
		x < imageWidthPx + kernelX.HorizontalRadius();
		x++)
	{
		float sumB = 0;
		float sumG = 0;
		float sumR = 0;

		for (int j = xOffsetPx; j < xOffsetPx + kernelX.Width(); j++)
		{
			sumB += kernelX(0, j - xOffsetPx) * expandedRow[j * BytePerPx];

			sumG += kernelX(0, j - xOffsetPx) * expandedRow[j * BytePerPx + 1];

			sumR += kernelX(0, j - xOffsetPx) * expandedRow[j * BytePerPx + 2];
		}

		outputRowsBuffer[currentRowOffset + (x - kernelX.HorizontalRadius()) * BytePerPx] = sumB;

		outputRowsBuffer[currentRowOffset + (x - kernelX.HorizontalRadius()) * BytePerPx + 1] = sumG;

		outputRowsBuffer[currentRowOffset + (x - kernelX.HorizontalRadius()) * BytePerPx + 2] = sumR;

		xOffsetPx++;
	}
}

void FilterImage(
	std::filesystem::path srcPath,
	std::filesystem::path destPath,
	const Kernel& kernelX,
	const Kernel& kernelY)
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
	if (kernelX.HorizontalRadius() > header.imageWidthPx ||
		kernelY.VerticalRadius() > header.imageHeightPx)
	{
		throw std::invalid_argument("Изображение слишком мало");
	}

	dest.write((char*)&header, sizeof(header));

	int imageWidthBytes = header.imageWidthPx * BytePerPx;
	int rowWidth = header.imageWidthPx * ChannelCount;
	int rowStrideBytes = (imageWidthBytes + 3) & ~3;
	int paddingBytesCount = rowStrideBytes - imageWidthBytes;

	// Длина расширенной отражёнными краевыми пикселями строки
	int expandedWidthBytes = imageWidthBytes + kernelX.HorizontalRadius() * 2 * BytePerPx;

	vector<uint8_t> srcRowBuffer(expandedWidthBytes);
	vector<float> convolutedXRows(rowWidth * kernelY.Height());
	vector<uint8_t> destRowBuffer(rowStrideBytes);

	src.seekg(header.imageOffsetBytes);
	// Формирование первоначального буфера строк с отражением
	// 1 0 0 1 2
	for (int bufferY = kernelY.VerticalRadius(); bufferY < kernelY.Height(); bufferY++)
	{
		int imageY = bufferY - kernelY.VerticalRadius();

		src.read((char*)&srcRowBuffer[kernelX.HorizontalRadius() * BytePerPx],
			imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);

		MirrorEdgePixelsInRow(srcRowBuffer, header.imageWidthPx, kernelX);

		ConvolutionX(
			srcRowBuffer,
			convolutedXRows,
			bufferY * rowWidth,
			kernelX,
			header.imageWidthPx
		);
	}

	// Отражение
	for (int bufferY = 0; bufferY < kernelY.VerticalRadius(); bufferY++)
	{
		int y = 2 * kernelY.VerticalRadius() - 1 - bufferY;

		std::copy(convolutedXRows.data() + y * rowWidth,
			convolutedXRows.data() + (y + 1) * rowWidth,
			convolutedXRows.data() + bufferY * rowWidth);
	}

	// Индекс первой строки в буфере
	int firstRowIndex = 0;
	int oldFirstRowIndex = 0;
	int bufIndexToCopy = 0;

	for (int y = kernelY.VerticalRadius();
		y < header.imageHeightPx + kernelY.VerticalRadius();
		y++)
	{
		for (int x = 0; x < header.imageWidthPx; x++)
		{
			float sumB = 0;
			float sumG = 0;
			float sumR = 0;

			// Свёртка
			for (int i = 0; i < kernelY.Height(); i++)
			{
				int currentRowIndex = (i + firstRowIndex) % kernelY.Height();
				
				sumB += kernelY(i, 0) *
					convolutedXRows[currentRowIndex * rowWidth + x * ChannelCount];

				sumG += kernelY(i, 0) *
					convolutedXRows[currentRowIndex * rowWidth + x * ChannelCount + 1];

				sumR += kernelY(i, 0) *
					convolutedXRows[currentRowIndex * rowWidth + x * ChannelCount + 2];
				
			}

			destRowBuffer[x * BytePerPx]
				= (uint8_t)std::clamp(sumB + 0.5f, 0.f, 255.f);

			destRowBuffer[x * BytePerPx + 1]
				= (uint8_t)std::clamp(sumG + 0.5f, 0.f, 255.f);

			destRowBuffer[x * BytePerPx + 2]
				= (uint8_t)std::clamp(sumR + 0.5f, 0.f, 255.f);
		}

		// Чтобы при чётных размерах условие отражение не срабатывало 
		// на 1 строку раньше и последняя строка тоже обрабатывалась 
		int imageY = y;
		if (kernelY.Height() % 2 == 0)
		{
			imageY = y - 1;
		}

		// Отражение 
		if (imageY >= header.imageHeightPx - 1)
		{
			if (imageY == header.imageHeightPx - 1)
			{
				oldFirstRowIndex = firstRowIndex;
				bufIndexToCopy = (firstRowIndex + kernelY.Height() - 1) % kernelY.Height();
			}
			if (bufIndexToCopy != firstRowIndex)
			{
				std::copy(convolutedXRows.data() + bufIndexToCopy * rowWidth,
					convolutedXRows.data() + (bufIndexToCopy + 1) * rowWidth,
					convolutedXRows.data() + firstRowIndex * rowWidth);
			}

			bufIndexToCopy--;
			if (bufIndexToCopy < 0)
			{
				bufIndexToCopy = kernelY.Height() - 1;
			}
		}
		else
		{
			src.read(
				(char*)&srcRowBuffer[kernelX.HorizontalRadius() * BytePerPx],
				imageWidthBytes);
			src.seekg(paddingBytesCount, std::ios_base::cur);

			MirrorEdgePixelsInRow(srcRowBuffer, header.imageWidthPx, kernelX);

			ConvolutionX(srcRowBuffer, convolutedXRows,
				firstRowIndex * rowWidth,
				kernelX,
				header.imageWidthPx);
		}

		// Вторая становится первой и т.д.
		firstRowIndex = (firstRowIndex + 1) % kernelY.Height();

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}