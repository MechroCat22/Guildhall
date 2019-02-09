/************************************************************************/
/* File: World.hpp
/* Author: Andrew Chase
/* Date: February 9th 2019
/* Description: Class to represent a game "world" of chunks
/************************************************************************/
#pragma once
#include <map>
#include "Engine/Math/IntVector2.hpp"

class Chunk;

class World
{
public:
	//-----Public Methods-----

	World();
	~World();

	void Update();
	void Render() const;

	void ActivateChunk(const IntVector2& chunkCoords);


private:
	//-----Private Data-----
	
	static constexpr int SEA_LEVEL = 40;
	static constexpr int BASE_ELEVATION = 64;
	static constexpr int NOISE_MAX_DEVIATION_FROM_BASE_ELEVATION = 10;

	static std::map<IntVector2, Chunk*> m_activeChunks;

};
