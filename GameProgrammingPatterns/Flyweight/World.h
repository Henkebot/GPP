#pragma once
#include "Terrain.h"

const int WIDTH = 20;
const int HEIGHT = 20;
const Texture GRASS_TEXTURE;
const Texture HILL_TEXTURE;
const Texture RIVER_TEXTURE;

class World
{
	// Only three instances of these
	Terrain m_grassTerrain;
	Terrain m_hillTerrain;
	Terrain m_riverTerrain;
	
	// We point at the already instanced terrain objects
	Terrain* m_tiles[WIDTH][HEIGHT];
public:
	World()
	: 
		m_grassTerrain(1, false, GRASS_TEXTURE),
		m_hillTerrain(3, false, HILL_TEXTURE),
		m_riverTerrain(2, true, RIVER_TEXTURE)
	{}
private:
	void _generateTerrain();

};
