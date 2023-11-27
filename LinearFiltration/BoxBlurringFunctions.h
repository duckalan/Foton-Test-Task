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

	// Буфер для загрузки 
	vector<uint8_t> srcRowBuffer(expandedWidthBytes);
	vector<uint32_t> convolutedXRows(rowWidth * kernelY.Height());

	// Формирование первоначального буфера строк с отражением (1 0 0 1 2)
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
	for (int x = 0; x < header.imageWidthPx; x++)
	{
		int nx = x * ChannelCount;
		for (int y = 0; y < kernelY.Height(); y++)
		{
			sumBuffer[nx] += convolutedXRows[y * rowWidth + nx];
			sumBuffer[nx + 1] += convolutedXRows[y * rowWidth + nx + 1];
			sumBuffer[nx + 2] += convolutedXRows[y * rowWidth + nx + 2];
		}
	}

	vector<uint8_t> destRowBuffer(rowStrideBytes);
	float divCoef = kernelX.Width() * kernelY.Height();
	int firstRowIndex = 0;
	int oldFirstRowIndex = 0;
	int bufIndexToCopy = 0;

	for (int y = kernelY.VerticalRadius();
		y < header.imageHeightPx + kernelY.VerticalRadius();
		y++)
	{
		// Запись в выходной буфер и вычитание верхней строки
		int offsetToTopRow = firstRowIndex * rowWidth;
		for (int x = 0; x < header.imageWidthPx; x++)
		{
			int nx = x * ChannelCount;

			destRowBuffer[x * BytePerPx] = (uint8_t)(sumBuffer[nx] / divCoef + 0.5f);
			destRowBuffer[x * BytePerPx + 1] = (uint8_t)(sumBuffer[nx + 1] / divCoef + 0.5f);
			destRowBuffer[x * BytePerPx + 2] = (uint8_t)(sumBuffer[nx + 2] / divCoef + 0.5f);

			sumBuffer[nx] -= convolutedXRows[offsetToTopRow + nx];
			sumBuffer[nx + 1] -= convolutedXRows[offsetToTopRow + nx + 1];
			sumBuffer[nx + 2] -= convolutedXRows[offsetToTopRow + nx + 2];
		}
		dest.write((char*)destRowBuffer.data(), rowStrideBytes);

		// Чтобы при чётных размерах условие отражение не срабатывало 
		// на 1 строку раньше и последняя строка тоже обрабатывалась 
		int imageY = kernelY.Height() % 2 == 0 ? y - 1 : y;

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
		// Запись новой строки
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

		// Прибавление нижней строки
		int offsetToBottomRow = ((firstRowIndex + kernelY.Height() - 1) % kernelY.Height()) * rowWidth;
		for (int x = 0; x < header.imageWidthPx; x++)
		{
			int nx = x * ChannelCount;
			sumBuffer[nx] += convolutedXRows[offsetToBottomRow + nx];
			sumBuffer[nx + 1] += convolutedXRows[offsetToBottomRow + nx + 1];
			sumBuffer[nx + 2] += convolutedXRows[offsetToBottomRow + nx + 2];
		}

		firstRowIndex = (firstRowIndex + 1) % kernelY.Height();
	}
}

void SumColsInRow(
	const vector<uint8_t>& mirroredRow,
	vector<uint32_t>& rowsOfSummedCols,
	vector<uint32_t>& rowsOfSummedSquaredCols,
	int currentRowOffset,
	const Kernel& kernelX,
	int imageWidthPx)
{
	uint32_t sumB = 0;
	uint32_t sumG = 0;
	uint32_t sumR = 0;
	
	uint32_t sumOfSquaredB = 0;
	uint32_t sumOfSquaredG = 0;
	uint32_t sumOfSquaredR = 0;

	// Проход для накопления первоначальной суммы
	for (int j = 0; j < kernelX.Width(); j++)
	{
		sumB += mirroredRow[j * BytePerPx];
		sumG += mirroredRow[j * BytePerPx + 1];
		sumR += mirroredRow[j * BytePerPx + 2];

		sumOfSquaredB += mirroredRow[j * BytePerPx] * mirroredRow[j * BytePerPx];
		sumOfSquaredG += mirroredRow[j * BytePerPx + 1] * mirroredRow[j * BytePerPx + 1];
		sumOfSquaredR += mirroredRow[j * BytePerPx + 2] * mirroredRow[j * BytePerPx + 2];
	}

	rowsOfSummedCols[currentRowOffset] = sumB;
	rowsOfSummedCols[currentRowOffset + 1] = sumG;
	rowsOfSummedCols[currentRowOffset + 2] = sumR;

	rowsOfSummedSquaredCols[currentRowOffset] = sumOfSquaredB;
	rowsOfSummedSquaredCols[currentRowOffset + 1] = sumOfSquaredG;
	rowsOfSummedSquaredCols[currentRowOffset + 2] = sumOfSquaredR;

	// Текущее смещение скользящего окна
	int xOffsetPx = 1;

	// Основная свёртка скользящим окном
	for (int x = kernelX.HorizontalRadius() + 1;
		x < imageWidthPx + kernelX.HorizontalRadius();
		x++)
	{
		int subtrahendBlueOffset = (xOffsetPx - 1) * BytePerPx;
		int addendBlueOffset = (xOffsetPx + kernelX.Width() - 1) * BytePerPx;

		sumB -= mirroredRow[subtrahendBlueOffset];
		sumB += mirroredRow[addendBlueOffset];

		sumG -= mirroredRow[subtrahendBlueOffset + 1];
		sumG += mirroredRow[addendBlueOffset + 1];

		sumR -= mirroredRow[subtrahendBlueOffset + 2];
		sumR += mirroredRow[addendBlueOffset + 2];

		sumOfSquaredB -= mirroredRow[subtrahendBlueOffset] * mirroredRow[subtrahendBlueOffset];
		sumOfSquaredB += mirroredRow[addendBlueOffset] * mirroredRow[addendBlueOffset];

		sumOfSquaredG -= mirroredRow[subtrahendBlueOffset + 1] * mirroredRow[subtrahendBlueOffset + 1];
		sumOfSquaredG += mirroredRow[addendBlueOffset + 1] * mirroredRow[addendBlueOffset + 1];

		sumOfSquaredR -= mirroredRow[subtrahendBlueOffset + 2] * mirroredRow[subtrahendBlueOffset + 2];
		sumOfSquaredR += mirroredRow[addendBlueOffset + 2] * mirroredRow[addendBlueOffset + 2];

		xOffsetPx++;

		int currentOutputBlueOffset = currentRowOffset + (x - kernelX.HorizontalRadius()) * BytePerPx;

		rowsOfSummedCols[currentOutputBlueOffset] = sumB;
		rowsOfSummedCols[currentOutputBlueOffset + 1] = sumG;
		rowsOfSummedCols[currentOutputBlueOffset + 2] = sumR;

		rowsOfSummedSquaredCols[currentOutputBlueOffset] = sumOfSquaredB;
		rowsOfSummedSquaredCols[currentOutputBlueOffset + 1] = sumOfSquaredG;
		rowsOfSummedSquaredCols[currentOutputBlueOffset + 2] = sumOfSquaredR;
	}
}

uint8_t Rmse(float sumOfSquares, float sum, float divCoef)
{
	float meanOfSquares = sumOfSquares / divCoef;
	float mean = sum / divCoef;

	return (uint8_t)std::clamp(sqrt(meanOfSquares - mean * mean) + 0.5f, 0.f, 255.f);
}

void MovingRmse(
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

	// Буфер длиной расширенной отражёнными краевыми пикселями строки
	vector<uint8_t> srcRowBuffer(imageWidthBytes + kernelX.HorizontalRadius() * 2 * BytePerPx);
	vector<uint32_t> rowsOfSummedCols(rowWidth * kernelY.Height());
	vector<uint32_t> rowsOfSummedSquaredCols(rowWidth * kernelY.Height());

	// Формирование первоначального буфера (- - 0 1 2)
	for (int bufferY = kernelY.VerticalRadius(); bufferY < kernelY.Height(); bufferY++)
	{
		int imageY = bufferY - kernelY.VerticalRadius();

		src.read((char*)&srcRowBuffer[kernelX.HorizontalRadius() * BytePerPx],
			imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);

		MirrorEdgePixelsInRow(srcRowBuffer, header.imageWidthPx, kernelX);

		SumColsInRow(
			srcRowBuffer,
			rowsOfSummedCols,
			rowsOfSummedSquaredCols,
			bufferY * rowWidth,
			kernelX,
			header.imageWidthPx
		);
	}

	// Отражение (1 0 0 1 2)
	for (int bufferY = 0; bufferY < kernelY.VerticalRadius(); bufferY++)
	{
		int y = 2 * kernelY.VerticalRadius() - 1 - bufferY;

		std::copy(rowsOfSummedCols.data() + y * rowWidth,
			rowsOfSummedCols.data() + (y + 1) * rowWidth,
			rowsOfSummedCols.data() + bufferY * rowWidth);

		std::copy(rowsOfSummedSquaredCols.data() + y * rowWidth,
			rowsOfSummedSquaredCols.data() + (y + 1) * rowWidth,
			rowsOfSummedSquaredCols.data() + bufferY * rowWidth);
	}

	// Накопление начальной суммы
	vector<uint32_t> sumBuffer(rowWidth);
	vector<uint32_t> sumOfSquaresBuffer(rowWidth);
	for (int x = 0; x < header.imageWidthPx; x++)
	{
		int nx = x * ChannelCount;
		for (int y = 0; y < kernelY.Height(); y++)
		{
			sumBuffer[nx] += rowsOfSummedCols[y * rowWidth + nx];
			sumBuffer[nx + 1] += rowsOfSummedCols[y * rowWidth + nx + 1];
			sumBuffer[nx + 2] += rowsOfSummedCols[y * rowWidth + nx + 2];

			sumOfSquaresBuffer[nx] += rowsOfSummedSquaredCols[y * rowWidth + nx];
			sumOfSquaresBuffer[nx + 1] += rowsOfSummedSquaredCols[y * rowWidth + nx + 1];
			sumOfSquaresBuffer[nx + 2] += rowsOfSummedSquaredCols[y * rowWidth + nx + 2];
		}
	}

	vector<uint8_t> destRowBuffer(rowStrideBytes);
	float divCoef = kernelX.Width() * kernelY.Height();
	int firstRowIndex = 0;
	int oldFirstRowIndex = 0;
	int bufIndexToCopy = 0;

	for (int y = kernelY.VerticalRadius();
		y < header.imageHeightPx + kernelY.VerticalRadius();
		y++)
	{
		// Запись в выходной буфер и вычитание верхней строки
		int offsetToTopRow = firstRowIndex * rowWidth;
		for (int x = 0; x < header.imageWidthPx; x++)
		{
			int nx = x * ChannelCount;

			destRowBuffer[x * BytePerPx] = Rmse(sumOfSquaresBuffer[nx], sumBuffer[nx], divCoef);
			destRowBuffer[x * BytePerPx + 1] = Rmse(sumOfSquaresBuffer[nx + 1], sumBuffer[nx + 1], divCoef);
			destRowBuffer[x * BytePerPx + 2] = Rmse(sumOfSquaresBuffer[nx + 2], sumBuffer[nx + 2], divCoef);

			sumBuffer[nx] -= rowsOfSummedCols[offsetToTopRow + nx];
			sumBuffer[nx + 1] -= rowsOfSummedCols[offsetToTopRow + nx + 1];
			sumBuffer[nx + 2] -= rowsOfSummedCols[offsetToTopRow + nx + 2];

			sumOfSquaresBuffer[nx] -= rowsOfSummedSquaredCols[offsetToTopRow + nx];// *squaredConvolutedXRows[offsetToTopRow + nx];
			sumOfSquaresBuffer[nx + 1] -= rowsOfSummedSquaredCols[offsetToTopRow + nx + 1];// *squaredConvolutedXRows[offsetToTopRow + nx + 1];
			sumOfSquaresBuffer[nx + 2] -= rowsOfSummedSquaredCols[offsetToTopRow + nx + 2];// *squaredConvolutedXRows[offsetToTopRow + nx + 2];
		}
		dest.write((char*)destRowBuffer.data(), rowStrideBytes);

		// Чтобы при чётных размерах условие отражение не срабатывало 
		// на 1 строку раньше и последняя строка тоже обрабатывалась 
		int imageY = kernelY.Height() % 2 == 0 ? y - 1 : y;

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
				std::copy(rowsOfSummedCols.data() + bufIndexToCopy * rowWidth,
					rowsOfSummedCols.data() + (bufIndexToCopy + 1) * rowWidth,
					rowsOfSummedCols.data() + firstRowIndex * rowWidth);

				std::copy(rowsOfSummedSquaredCols.data() + bufIndexToCopy * rowWidth,
					rowsOfSummedSquaredCols.data() + (bufIndexToCopy + 1) * rowWidth,
					rowsOfSummedSquaredCols.data() + firstRowIndex * rowWidth);
			}

			bufIndexToCopy--;
			if (bufIndexToCopy < 0)
			{
				bufIndexToCopy = kernelY.Height() - 1;
			}
		}
		// Запись новой строки
		else
		{
			src.read(
				(char*)&srcRowBuffer[kernelX.HorizontalRadius() * BytePerPx],
				imageWidthBytes);
			src.seekg(paddingBytesCount, std::ios_base::cur);

			MirrorEdgePixelsInRow(srcRowBuffer, header.imageWidthPx, kernelX);

			SumColsInRow(
				srcRowBuffer, 
				rowsOfSummedCols, 
				rowsOfSummedSquaredCols,
				firstRowIndex * rowWidth,
				kernelX,
				header.imageWidthPx);
		}

		// Прибавление нижней строки
		int offsetToBottomRow = ((firstRowIndex + kernelY.Height() - 1) % kernelY.Height()) * rowWidth;
		for (int x = 0; x < header.imageWidthPx; x++)
		{
			int nx = x * ChannelCount;
			sumBuffer[nx] += rowsOfSummedCols[offsetToBottomRow + nx];
			sumBuffer[nx + 1] += rowsOfSummedCols[offsetToBottomRow + nx + 1];
			sumBuffer[nx + 2] += rowsOfSummedCols[offsetToBottomRow + nx + 2];

			sumOfSquaresBuffer[nx] += rowsOfSummedSquaredCols[offsetToBottomRow + nx];
			sumOfSquaresBuffer[nx + 1] += rowsOfSummedSquaredCols[offsetToBottomRow + nx + 1];
			sumOfSquaresBuffer[nx + 2] += rowsOfSummedSquaredCols[offsetToBottomRow + nx + 2];
		}

		firstRowIndex = (firstRowIndex + 1) % kernelY.Height();
	}
}