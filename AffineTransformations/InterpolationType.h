#pragma once

enum class InterpolationType
{
	NearestNeighbor,
	Bilinear,
	BiCubic,

	// Фильтр Ланцоша с окном радиусом 2 пикселя (4x4)
	Lanczos2,

	// Фильтр Ланцоша с окном радиусом 3 пикселя (6x6)
	Lanczos3
};