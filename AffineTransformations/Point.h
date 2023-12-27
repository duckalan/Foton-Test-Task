#pragma once

struct Point
{
	Point()
	{
		x = 0.f;
		y = 0.f;
	}

	Point(float x, float y)
	{
		this->x = x;
		this->y = y;
	}

	float x;
	float y;
};