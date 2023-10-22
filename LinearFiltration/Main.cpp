#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <numeric>

#include "BmpHeader.h"

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

const int BytePerPx = 3;

/*
	линейная фильтрация - свёртка изображения с матрицой фильтра
	вокруг пикселя берётся матрица с весами, например, 5x5,
	затем накапливаем сумму произведения. Перемножаем каждый пиксель с весом,
	и суммируем всё в окне. Результат в пиксель картинки. С чётными матрицами
	результат будет смещаться на пол пикселя. Будет нечётная.
	С краевыми пикселями (краевые эффекты) надо дополнять данные.
	Либо дублировать крайние строки и столбцы, либо зеркально отражать картинку.
	Коэффициенты - скользящее среднее. Шаг скольжения 1. Получим рамзытую картинку
	исходных размеров. Если её просто прорядить (взять пиксели), получим то,
	что раньше было.

	Как вычислять. Для формирования нужно по n строк, одновременно. Перечитывать
	n-1 строк и столбцов не надо. Массив на n строк одномерный, затем, на следующей строке
	0 строка не нужна, надо новую записать поверх 0. Затем поверх 1, поверх 2 и т.д.
	Циклический индекс от 0 до n-1, (i + 1) % n, по нему подгружаем строку в буффер строк

	При свёртке со строками с i-й строкой матрицы умножаем буффер с i + i_0,
	i_0 - смещение, показывающее индекс хранения 1-й строки, и в это i_0 записываем.
	это 1 раз на строку(i + i_0) % n, остаток всего n раз
	i->(i + i_0) % n в буфере
	обновление буфера - вместо i0 грузим следующую строку, и i0 = i0++ % n;
	с учётом смещения обработаем
	I_0 = 0 изначально. Затем цикл по всем строкам буфера
	*/

void MirrorEdgePixelsInRow(
	vector<uint8_t>& expandedRows,
	int rowIndex,
	int kernelDim)
{
	int kernelRadiusPx = (kernelDim - 1) / 2;
	int expandedWidthBytes = expandedRows.size() / kernelDim;
	int currentRowIndexBytes = expandedWidthBytes * rowIndex;

	// Левый конец
	for (int x = 0; x < kernelRadiusPx; x++)
	{
		expandedRows[currentRowIndexBytes + x * BytePerPx] =
			expandedRows[currentRowIndexBytes + (kernelDim - 1 - x) * BytePerPx];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 1] =
			expandedRows[currentRowIndexBytes + (kernelDim - 1 - x) * BytePerPx + 1];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 2] =
			expandedRows[currentRowIndexBytes + (kernelDim - 1 - x) * BytePerPx + 2];
	}

	// Правый конец
	int origWidthPx = (expandedWidthBytes - kernelRadiusPx * 2 * BytePerPx) / BytePerPx;
	int t = kernelDim - 2;

	for (int x = origWidthPx + kernelRadiusPx * 2 - 1;
		 x >= origWidthPx + kernelRadiusPx - 1;
		 x--)
	{
		expandedRows[currentRowIndexBytes + x * BytePerPx] =
			expandedRows[currentRowIndexBytes + (x - t - 1) * BytePerPx];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 1] =
			expandedRows[currentRowIndexBytes + (x - t - 1) * BytePerPx + 1];

		expandedRows[currentRowIndexBytes + x * BytePerPx + 2] =
			expandedRows[currentRowIndexBytes + (x - t - 1) * BytePerPx + 2];

		t -= 2;
	}
}


void FilterImageWithMirroring(
	std::filesystem::path srcPath,
	std::filesystem::path destPath,
	vector<float> kernel)
{
	std::ifstream src(srcPath, std::ios::binary);
	std::ofstream dest(destPath, std::ios::binary);

	if (!src.is_open())
	{
		std::string errorMessage = "Can't find or open file with input path: ";
		throw std::invalid_argument(errorMessage + srcPath.string());
	}
	if (!dest.is_open())
	{
		std::string errorMessage = "Can't save file with output path: ";
		throw std::invalid_argument(errorMessage + destPath.string());
	}

	BmpHeader header{};

	src.read((char*)&header, sizeof(header));
	dest.write((char*)&header, sizeof(header));

	int imageWidthBytes = header.imageWidthPx * BytePerPx;
	int rowStrideBytes = (imageWidthBytes + 3) & ~3;
	int paddingBytesCount = rowStrideBytes - imageWidthBytes;

	// Размерность ядра
	int kernelDim = (int)sqrt(kernel.size());

	// Расстояние от центрального элемента до конца стороны
	int kernelRadiusPx = (kernelDim - 1) / 2;

	// Нормировочный коэффициент ядра
	float div = 0;

	for (int i = 0; i < kernel.size(); i++)
	{
		div += kernel[i];
	}
	if (div == 0)
	{
		div = 1;
	}

	// Длина расширенной отражёнными краевыми пикселями строки
	int expandedWidthBytes = imageWidthBytes + kernelRadiusPx * 2 * BytePerPx;

	// Дополненные отражённые пиксели будут в начале строки и конце строки
	vector<uint8_t> srcRowsBufferEx(expandedWidthBytes * kernelDim);
	vector<uint8_t> destRowBuffer(rowStrideBytes);

	// Формирование первоначального буфера из 
	// первых kernelDim строк
	src.read((char*)&srcRowsBufferEx[kernelRadiusPx * BytePerPx],
		imageWidthBytes);
	src.seekg(paddingBytesCount, std::ios_base::cur);
	MirrorEdgePixelsInRow(srcRowsBufferEx, 0, kernelDim);

	for (int y = 1; y < kernelDim; y++)
	{
		src.read((char*)&srcRowsBufferEx[kernelRadiusPx * BytePerPx + expandedWidthBytes * y],
			imageWidthBytes);

		src.seekg(paddingBytesCount, std::ios_base::cur);

		MirrorEdgePixelsInRow(srcRowsBufferEx, y, kernelDim);
	}

	// Обработка первых kernelRadiusPx строк с отражением
	for (int y = 0; y < kernelRadiusPx; y++)
	{
		int xWindowOffsetPx = 0;
		for (int x = kernelRadiusPx; x < header.imageWidthPx + kernelRadiusPx; x++)
		{
			// Индекс текущего веса в ядре
			int kernelIndex = 0;

			float sumB = 0;
			float sumG = 0;
			float sumR = 0;

			// Свёртка
			for (int i = 0; i < kernelDim; i++)
			{
				// Отражение
				int currentY = 0;
				if (y < kernelRadiusPx && i < kernelRadiusPx - y)
				{
					currentY = kernelRadiusPx - i + y;
				}
				else
				{
					currentY = i - kernelRadiusPx + y;
				}

				for (int j = xWindowOffsetPx; j < xWindowOffsetPx + kernelDim; j++)
				{
					sumB += kernel[kernelIndex] *
						srcRowsBufferEx[currentY * expandedWidthBytes + j * BytePerPx];

					sumG += kernel[kernelIndex] *
						srcRowsBufferEx[currentY * expandedWidthBytes + j * BytePerPx + 1];

					sumR += kernel[kernelIndex] *
						srcRowsBufferEx[currentY * expandedWidthBytes + j * BytePerPx + 2];

					kernelIndex++;
				}
			}

			// x - kernelRadiusPx, т.к. основное изображение смещено на kernelRadiusPx
			destRowBuffer[(x - kernelRadiusPx) * BytePerPx] = (uint8_t)std::clamp(sumB / div, 0.f, 255.f);
			destRowBuffer[(x - kernelRadiusPx) * BytePerPx + 1] = (uint8_t)std::clamp(sumG / div, 0.f, 255.f);
			destRowBuffer[(x - kernelRadiusPx) * BytePerPx + 2] = (uint8_t)std::clamp(sumR / div, 0.f, 255.f);

			xWindowOffsetPx++;
		}

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}

	// Обработка строк до kernelRadiusPx с конца без отражений строк
	int firstRowIndex = 0;
	int t = 0;
	for (int y = kernelRadiusPx; y < header.imageHeightPx; y++)
	{
		int xWindowOffsetPx = 0;
		for (int x = kernelRadiusPx; x < header.imageWidthPx + kernelRadiusPx; x++)
		{
			// Индекс текущего веса в ядре
			int kernelIndex = 0;

			float sumB = 0;
			float sumG = 0;
			float sumR = 0;

			// Свёртка
			for (int i = 0; i < kernelDim; i++)
			{
				int currentY = (i + firstRowIndex) % kernelDim;
				if (y > header.imageHeightPx - kernelRadiusPx - 1 && 
					i > kernelDim - 1 - ( kernelRadiusPx - (header.imageHeightPx - y - 1)))
				{
					currentY = (i + firstRowIndex - kernelRadiusPx + t) % kernelDim;
				}

				for (int j = xWindowOffsetPx; j < xWindowOffsetPx + kernelDim; j++)
				{
					sumB += kernel[kernelIndex] *
						srcRowsBufferEx[currentY * expandedWidthBytes + j * BytePerPx];

					sumG += kernel[kernelIndex] *
						srcRowsBufferEx[currentY * expandedWidthBytes + j * BytePerPx + 1];

					sumR += kernel[kernelIndex] *
						srcRowsBufferEx[currentY * expandedWidthBytes + j * BytePerPx + 2];

					kernelIndex++;
				}
			}

			if (sumR != 0 && sumB < 10 && sumG < 10)
			{
				int a = 65;
				int b = a;
			}
			destRowBuffer[(x - kernelRadiusPx) * BytePerPx] = (uint8_t)std::clamp(sumB / div, 0.f, 255.f);
			destRowBuffer[(x - kernelRadiusPx) * BytePerPx + 1] = (uint8_t)std::clamp(sumG / div, 0.f, 255.f);
			destRowBuffer[(x - kernelRadiusPx) * BytePerPx + 2] = (uint8_t)std::clamp(sumR / div, 0.f, 255.f);

			xWindowOffsetPx++;
		}

		if (y > header.imageHeightPx - kernelRadiusPx - 1)
		{
			t++;
		}

		// Запись следующей строки на место первой
		src.read((char*)&srcRowsBufferEx[kernelRadiusPx * BytePerPx + expandedWidthBytes * firstRowIndex], imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);
		MirrorEdgePixelsInRow(srcRowsBufferEx, firstRowIndex, kernelDim);

		// Вторая становится первой и т.д.
		firstRowIndex = (firstRowIndex + 1) % kernelDim;

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}


void FilterImageNoMirror(
	std::filesystem::path srcPath,
	std::filesystem::path destPath,
	vector<float> kernel)
{
	std::ifstream src(srcPath, std::ios::binary);
	std::ofstream dest(destPath, std::ios::binary);

	if (!src.is_open())
	{
		std::string errorMessage = "Can't find or open file with input path: ";
		throw std::invalid_argument(errorMessage + srcPath.string());
	}
	if (!dest.is_open())
	{
		std::string errorMessage = "Can't save file with output path: ";
		throw std::invalid_argument(errorMessage + destPath.string());
	}

	BmpHeader header{};

	src.read((char*)&header, sizeof(header));
	dest.write((char*)&header, sizeof(header));

	int imageWidthBytes = header.imageWidthPx * BytePerPx;
	int rowStrideBytes = (imageWidthBytes + 3) & ~3;
	int paddingBytesCount = rowStrideBytes - imageWidthBytes;

	// Размерность ядра
	int kernelDim = (int)sqrt(kernel.size());

	// Расстояние от центрального элемента до конца стороны
	int kernelRadiusPx = (kernelDim - 1) / 2;

	// Нормировочный коэффициент ядра
	float div = 0;

	for (int i = 0; i < kernel.size(); i++)
	{
		div += kernel[i];
	}

	vector<uint8_t> srcRowsBuffer((size_t)imageWidthBytes * kernelDim);
	vector<uint8_t> destRowBuffer(rowStrideBytes);

	// Заполнение первых пустых строк
	for (int y = 0; y < kernelRadiusPx; y++)
	{
		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}

	// Формирование первоначального буфера из 
	// первых kernelDim строк
	for (int i = 0; i < kernelDim; i++)
	{
		src.read((char*)&srcRowsBuffer[imageWidthBytes * i],
			imageWidthBytes);

		src.seekg(paddingBytesCount, std::ios_base::cur);
	}

	int firstRowIndex = 0;
	for (int y = kernelRadiusPx; y < header.imageHeightPx - kernelRadiusPx; y++)
	{
		int xWindowOffsetPx = 0;
		for (int x = kernelRadiusPx; x < header.imageWidthPx - kernelRadiusPx; x++)
		{
			// Индекс текущего веса в ядре
			int kernelIndex = 0;

			float sumB = 0;
			float sumG = 0;
			float sumR = 0;

			// Свёртка
			for (int i = 0; i < kernelDim; i++)
			{
				int currentRowIndex = (i + firstRowIndex) % kernelDim;

				for (int j = xWindowOffsetPx; j < xWindowOffsetPx + kernelDim; j++)
				{
					sumB += kernel[kernelIndex] *
						srcRowsBuffer[currentRowIndex * imageWidthBytes + j * BytePerPx];

					sumG += kernel[kernelIndex] *
						srcRowsBuffer[currentRowIndex * imageWidthBytes + j * BytePerPx + 1];

					sumR += kernel[kernelIndex] *
						srcRowsBuffer[currentRowIndex * imageWidthBytes + j * BytePerPx + 2];

					kernelIndex++;
				}
			}

			destRowBuffer[x * BytePerPx] = (uint8_t)std::clamp(sumB / div, 0.f, 255.f);
			destRowBuffer[x * BytePerPx + 1] = (uint8_t)std::clamp(sumG / div, 0.f, 255.f);
			destRowBuffer[x * BytePerPx + 2] = (uint8_t)std::clamp(sumR / div, 0.f, 255.f);

			xWindowOffsetPx++;
		}

		// Запись следующей строки на место первой
		src.read((char*)&srcRowsBuffer[imageWidthBytes * firstRowIndex], imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);

		// Вторая становится первой и т.д.
		firstRowIndex = (firstRowIndex + 1) % kernelDim;

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}

	// Заполнение последних пустых строк
	std::fill(std::begin(destRowBuffer), std::end(destRowBuffer), 0u);
	for (int y = 0; y < kernelRadiusPx; y++)
	{
		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}

int main()
{
	const int n = 5;
	//vector<float> kernel(n * n, 1.f);
	vector<float> kernel
	{
		0, -1, 0,
		-1, 4, -1,
		0, -1, 0,
	};

	try
	{
		auto now = high_resolution_clock::now();

		FilterImageWithMirroring(
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