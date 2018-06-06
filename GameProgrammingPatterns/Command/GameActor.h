#pragma once
#include <iostream>
class GameActor
{
private:
	float x;
	float y;
public:
	GameActor() : x(0), y(0) {}
	void Jump()
	{
		std::cout << "GameActor-Jump executed!\n";
	}
	void Fire()
	{
		std::cout << "GameActor-Fire executed!\n";
	}

	void Move(float x_, float y_)
	{
		x = x_;
		y = y_;
		std::cout << "New Position (" << x << "," << y << ")\n";
	}
	inline float getX() { return x; }
	inline float getY() { return y; }
};
