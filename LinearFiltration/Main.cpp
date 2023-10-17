#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>

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

void FilterImage(
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
	vector<uint8_t> destRowBuffer(rowStrideBytes);

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

	// Длина расширенной отражёнными краевыми пикселями строки
	int expandedWidthBytes = imageWidthBytes + kernelRadiusPx * 2 * BytePerPx;

	// Дополненные отражённые пиксели будут в начале строки и конце строки
	vector<uint8_t> srcRowsBuffer(
		(size_t)expandedWidthBytes * kernelDim);

	// Формирование первоначального буфера из первых kernelDim строк
	// с оствавлением места под отражённые пиксели с конца одной
	// и начала другой строки
	for (int y = 0; y < kernelDim; y++)
	{
		src.read((char*)&srcRowsBuffer[
			expandedWidthBytes * y + kernelRadiusPx * BytePerPx],
			imageWidthBytes);

		src.seekg(paddingBytesCount, std::ios_base::cur);

		MirrorEdgePixelsInRow(srcRowsBuffer, y, kernelDim);
	}

	// Индекс текущего веса в ядре
	int kernelIndex = 0;
	float sumB = 0;
	float sumG = 0;
	float sumR = 0;

	for (int y = 0; y < header.imageHeightPx; y++)
	{
		for (int x = kernelRadiusPx; x < header.imageWidthPx + kernelRadiusPx; x++)
		{
			int currY = y;
			if (y < kernelRadiusPx)
			{
				currY = kernelRadiusPx - y;
			}

			for (int i = -kernelRadiusPx; i <= kernelRadiusPx; i++)
			{
				for (int j = -kernelRadiusPx; j <= kernelRadiusPx; j++)
				{
					sumB += kernel[kernelIndex] *
						srcRowsBuffer[(y - i) * expandedWidthBytes + (x - j) * BytePerPx];

					sumG += kernel[kernelIndex] *
						srcRowsBuffer[(y - i) * expandedWidthBytes + (x - j) * BytePerPx + 1];

					sumR += kernel[kernelIndex] *
						srcRowsBuffer[(y - i) * expandedWidthBytes + (x - j) * BytePerPx + 2];

					kernelIndex++;
				}
			}
			kernelIndex = 0;
		}
	}


	for (int y = 0; y < header.imageHeightPx; y++)
	{
		// Цикл по основным, неотражённым пикселям строки
		for (int x = kernelRadiusPx; x < header.imageWidthPx + kernelRadiusPx; x++)
		{
			for (int i = -kernelRadiusPx; i < kernelRadiusPx; i++)
			{
				for (int j = -kernelRadiusPx; j < kernelRadiusPx; j++)
				{
					//kernel[(kernelDim - i) * kernelDim - 1 - j] 

					sumB += kernel[kernelIndex] *
						srcRowsBuffer[(y - i) * imageWidthBytes + (x - j) * BytePerPx];

					sumG += kernel[kernelIndex] *
						srcRowsBuffer[(y - i) * imageWidthBytes + (x - j) * BytePerPx + 1];

					sumR += kernel[kernelIndex] *
						srcRowsBuffer[(y - i) * imageWidthBytes + (x - j) * BytePerPx + 2];

					kernelIndex++;
				}
			}
		}
	}

	// Обработка отражённых первых строк, выходящих за изображение
	for (int y = kernelRadiusPx; y >= 0; y--)
	{
		// Цикл по основным, неотражённым пикселям строки
		for (int x = kernelRadiusPx; x < header.imageWidthPx + kernelRadiusPx; x++)
		{
			// Свёртка
			for (int i = 0; i < kernelRadiusPx; i++)
			{
				for (int i = -kernelRadiusPx; i < kernelRadiusPx; i++)
				{

				}

				for (int j = -kernelRadiusPx; j < kernelRadiusPx; i++)
				{
					//kernel[(kernelDim - i) * kernelDim - 1 - j] 

					sumB += kernel[kernelIndex] *
						srcRowsBuffer[(y + i) * imageWidthBytes + (j + x) * BytePerPx];

					sumG += kernel[kernelIndex] *
						srcRowsBuffer[y * imageWidthBytes + (j + x) * BytePerPx + 1];

					sumR += kernel[kernelIndex] *
						srcRowsBuffer[y * imageWidthBytes + (j + x) * BytePerPx + 2];

					kernelIndex--;
				}

				for (int j = 0; j < kernelDim; j++)
				{
					sumB += kernel[(kernelDim - i) * kernelDim - 1 - j] *
						srcRowsBuffer[(y + i) * imageWidthBytes + (x - j) * BytePerPx];

					sumG += kernel[(kernelDim - i) * kernelDim - 1 - j] *
						srcRowsBuffer[y * imageWidthBytes + x + 1];

					sumR += kernel[(kernelDim - i) * kernelDim - 1 - j] *
						srcRowsBuffer[y * imageWidthBytes + x + 2];

					kernelIndex++;
				}
			}
		}
	}

	// Обработка оставшихся первых строк
	// ...

	for (int y = 0; y < kernelDim; y++)
	{
		for (int x = 0; x < header.imageWidthPx; x++)
		{
			int kernelIndex = 0;
			float sumB = 0;
			float sumG = 0;
			float sumR = 0;

			for (int i = 0; i < kernelDim; i++)
			{
				for (int j = 0; j < kernelDim; j++)
				{

				}
			}


			destRowBuffer[y * imageWidthBytes + x] = (uint8_t)(sumB / div + 0.5f);
			destRowBuffer[y * imageWidthBytes + x + 1] = (uint8_t)(sumG / div + 0.5f);
			destRowBuffer[y * imageWidthBytes + x + 2] = (uint8_t)(sumR / div + 0.5f);
		}
	}

	int firstRowOffset = 0;

	for (int y = kernelDim; y < header.imageHeightPx; y++)
	{
		firstRowOffset = (firstRowOffset + 1) % kernelDim;
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

	// Формирование первоначального буфера из первых kernelDim строк
	for (int y = 0; y < kernelDim; y++)
	{
		src.read((char*)&srcRowsBuffer[imageWidthBytes * y],
			imageWidthBytes);

		src.seekg(paddingBytesCount, std::ios_base::cur);
	}

	int firstRowIndex = -1;

	for (int y = kernelRadiusPx; y < header.imageHeightPx - kernelRadiusPx; y++)
	{
		if (y == kernelRadiusPx)
		{
			firstRowIndex = 0;
		}
		else
		{
			firstRowIndex = ((y - kernelRadiusPx) + 1) % kernelDim;
		}

		for (int x = kernelRadiusPx; x < header.imageWidthPx - kernelRadiusPx; x++)
		{
			// Индекс текущего веса в ядре
			int kernelIndex = 0;

			float sumB = 0;
			float sumG = 0;
			float sumR = 0;

			/*for (int i = -kernelRadiusPx; i <= kernelRadiusPx; i++)
			{
				for (int j = -kernelRadiusPx; j <= kernelRadiusPx; j++)
				{
					sumB += kernel[(i + kernelRadiusPx) * kernelDim + (j + kernelRadiusPx)]
						*
				}
			}*/
		

			// Свёртка
			for (int i = -kernelRadiusPx; i <= kernelRadiusPx; i++)
			{
				for (int j = -kernelRadiusPx; j <= kernelRadiusPx; j++)
				{

					sumB += kernel[(i + kernelRadiusPx) * kernelDim + (j + kernelRadiusPx)] *
						srcRowsBuffer[(y + firstRowIndex - i) * imageWidthBytes + (x - j) * BytePerPx];

					sumG += kernel[(i + kernelRadiusPx) * kernelDim + (j + kernelRadiusPx)] *
						srcRowsBuffer[(y + firstRowIndex - i) * imageWidthBytes + (x - j) * BytePerPx + 1];

					sumR += kernel[(i + kernelRadiusPx) * kernelDim + (j + kernelRadiusPx)] *
						srcRowsBuffer[(y + firstRowIndex - i) * imageWidthBytes + (x - j) * BytePerPx + 2];

					kernelIndex++;
				}
			}

			destRowBuffer[x * BytePerPx] = (uint8_t)(sumB / div + 0.5f);
			destRowBuffer[x * BytePerPx + 1] = (uint8_t)(sumG / div + 0.5f);
			destRowBuffer[x * BytePerPx + 2] = (uint8_t)(sumR / div + 0.5f);
		}

		src.read((char*)&srcRowsBuffer[imageWidthBytes * firstRowIndex],
			imageWidthBytes);
		src.seekg(paddingBytesCount, std::ios_base::cur);

		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}

void FilterImageSimple(
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

	// Формирование первоначального буфера из первых kernelDim строк
	
	for (int y = 0; y < kernelRadiusPx; y++)
	{
		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}

	int cnt1 = 0;
	for (int y = kernelRadiusPx; y < header.imageHeightPx - kernelRadiusPx; y++)
	{
		src.seekg(54 + rowStrideBytes * cnt1++);
		// Чтение kernelDim строк
		for (int i = 0; i < kernelDim; i++)
		{
			src.read((char*)&srcRowsBuffer[imageWidthBytes * i],
				imageWidthBytes);

			src.seekg(paddingBytesCount, std::ios_base::cur);
		}

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
				for (int j = xWindowOffsetPx; j < xWindowOffsetPx + kernelDim; j++)
				{
					sumB += kernel[kernelIndex] *
						srcRowsBuffer[(i) * imageWidthBytes + (j) * BytePerPx];

					sumG += kernel[kernelIndex] *
						srcRowsBuffer[(i) * imageWidthBytes + (j) * BytePerPx + 1];

					sumR += kernel[kernelIndex] *
						srcRowsBuffer[(i) * imageWidthBytes + (j) * BytePerPx + 2];

					kernelIndex++;
				}
			}
			xWindowOffsetPx++;
			destRowBuffer[x * BytePerPx] = (uint8_t)(sumB / div + 0.5f);
			destRowBuffer[x * BytePerPx + 1] = (uint8_t)(sumG / div + 0.5f);
			destRowBuffer[x * BytePerPx + 2] = (uint8_t)(sumR / div + 0.5f);
		}
		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}

	std::fill(std::begin(destRowBuffer), std::end(destRowBuffer), 0u);
	for (int y = 0; y < kernelRadiusPx; y++)
	{
		dest.write((char*)destRowBuffer.data(), rowStrideBytes);
	}
}

int main()
{
	const int n = 3;
	vector<float> kernel(n * n, 1.f);

	try
	{
		auto now = high_resolution_clock::now();

		FilterImageSimple(
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