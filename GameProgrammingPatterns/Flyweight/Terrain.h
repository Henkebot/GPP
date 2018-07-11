#pragma once

class Texture
{};

class Terrain
{
public:
	Terrain(int movementCost,
		bool isWater,
		Texture texture)
		: m_movementCost(movementCost),
		m_isWater(isWater),
		m_texture(texture)
	{}
	inline int getMovementCost() const { return m_movementCost; }
	inline bool isWater() const { return m_isWater; }
	inline const Texture& getTexture() const { return m_texture; }
private:
	int m_movementCost;
	bool m_isWater;
	Texture m_texture;
};