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
//const int BytePerPx = 3;
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
	vector<uint32_t>& outputRowsBuffer,
	int currentRowOffset,
	const Kernel& kernelX,
	int imageWidthPx)
{
	uint32_t sumB = 0;
	uint32_t sumG = 0;
	uint32_t sumR = 0;

	// Проход для накопления первоначальной суммы
	// здесь будто бы x = kernelX.HorizontalRadius() 
	for (int j = 0; j < kernelX.Width(); j++)
	{
		sumB += expandedRow[j * BytePerPx];
		sumG += expandedRow[j * BytePerPx + 1];
		sumR += expandedRow[j * BytePerPx + 2];
	}

	outputRowsBuffer[currentRowOffset] = sumB;
	outputRowsBuffer[currentRowOffset + 1] = sumG;
	outputRowsBuffer[currentRowOffset + 2] = sumR;

	// Текущее смещение скользящего окна
	int xOffsetPx = 1;

	// Основная свёртка скользящим окном
	for (int x = kernelX.HorizontalRadius() + 1;
		x < imageWidthPx + kernelX.HorizontalRadius();
		x++)
	{
		sumB -= expandedRow[(xOffsetPx - 1) * BytePerPx];
		sumB += expandedRow[(xOffsetPx + kernelX.Width() - 1) * BytePerPx];

		sumG -= expandedRow[(xOffsetPx - 1) * BytePerPx + 1];
		sumG += expandedRow[(xOffsetPx + kernelX.Width() - 1) * BytePerPx + 1];

		sumR -= expandedRow[(xOffsetPx - 1) * BytePerPx + 2];
		sumR += expandedRow[(xOffsetPx + kernelX.Width() - 1) * BytePerPx + 2];

		outputRowsBuffer[currentRowOffset + (x - kernelX.HorizontalRadius()) * BytePerPx] = sumB;
		outputRowsBuffer[currentRowOffset + (x - kernelX.HorizontalRadius()) * BytePerPx + 1] = sumG;
		outputRowsBuffer[currentRowOffset + (x - kernelX.HorizontalRadius()) * BytePerPx + 2] = sumR;

		xOffsetPx++;
	}
}

void BoxBlur(
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
	vector<uint32_t> convolutedXRows(rowWidth * kernelY.Height());
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

	// Накопление начальной суммы
	vector<uint32_t> sumBuffer(rowWidth);
	float divCoef = kernelX.Width() * kernelY.Height();

	for (int x = 0; x < header.imageWidthPx; x++)
	{
		int nx = x * ChannelCount;
		for (int y = 0; y < kernelY.Height(); y++)
		{
			sumBuffer[nx] += convolutedXRows[y * rowWidth + nx];
			sumBuffer[nx + 1] += convolutedXRows[y * rowWidth + nx + 1];
			sumBuffer[nx + 2] += convolutedXRows[y * rowWidth + nx + 2];
		}

		destRowBuffer[x * BytePerPx] = (uint8_t)(sumBuffer[nx] / divCoef + 0.5f);
		destRowBuffer[x * BytePerPx + 1] = (uint8_t)(sumBuffer[nx + 1] / divCoef + 0.5f);
		destRowBuffer[x * BytePerPx + 2] = (uint8_t)(sumBuffer[nx + 2] / divCoef + 0.5f);

		// Заранее вычитаем верхнюю строку
		sumBuffer[nx] -= convolutedXRows[nx];
		sumBuffer[nx + 1] -= convolutedXRows[nx + 1];
		sumBuffer[nx + 2] -= convolutedXRows[nx + 2];
	}
	//std::fill(begin(sumBuffer), end(sumBuffer), 0u);

	dest.write((char*)destRowBuffer.data(), rowStrideBytes);

	src.read(
		(char*)&srcRowBuffer[kernelX.HorizontalRadius() * BytePerPx],
		imageWidthBytes);
	src.seekg(paddingBytesCount, std::ios_base::cur);

	MirrorEdgePixelsInRow(srcRowBuffer, header.imageWidthPx, kernelX);

	ConvolutionX(srcRowBuffer, convolutedXRows,
		0,
		kernelX,
		header.imageWidthPx);

	// Индекс первой строки в буфере
	int firstRowIndex = 1;
	int oldFirstRowIndex = 0;
	int bufIndexToCopy = 0;

	for (int y = kernelY.VerticalRadius() + 1;
		y < header.imageHeightPx + kernelY.VerticalRadius();
		y++)
	{
		int offsetToTopRow = firstRowIndex * rowWidth;
		int offsetToBottomRow = ((firstRowIndex + kernelY.Height() - 1) % kernelY.Height()) * rowWidth;

		for (int x = 0; x < header.imageWidthPx; x++)
		{
			int nx = x * ChannelCount;
			// Прибавление нижней строки
			sumBuffer[nx] += convolutedXRows[offsetToBottomRow + nx];
			sumBuffer[nx + 1] += convolutedXRows[offsetToBottomRow + nx + 1];
			sumBuffer[nx + 2] += convolutedXRows[offsetToBottomRow + nx + 2];

			destRowBuffer[x * BytePerPx] = (uint8_t)(sumBuffer[nx] / divCoef + 0.5f);
			destRowBuffer[x * BytePerPx + 1] = (uint8_t)(sumBuffer[nx + 1] / divCoef + 0.5f);
			destRowBuffer[x * BytePerPx + 2] = (uint8_t)(sumBuffer[nx + 2] / divCoef + 0.5f);

			// Вычитание верхней строки
			sumBuffer[nx] -= convolutedXRows[offsetToTopRow + nx];
			sumBuffer[nx + 1] -= convolutedXRows[offsetToTopRow + nx + 1];
			sumBuffer[nx + 2] -= convolutedXRows[offsetToTopRow + nx + 2];
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