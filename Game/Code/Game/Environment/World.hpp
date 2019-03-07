/************************************************************************/
/* File: World.hpp
/* Author: Andrew Chase
/* Date: February 9th 2019
/* Description: Class to represent a game "world" of chunks
/************************************************************************/
#pragma once
#include <map>
#include <deque>
#include "Engine/Math/IntVector2.hpp"
#include "Game/Environment/BlockLocator.hpp"

// For raycasting into blocks in the world
struct RaycastResult_t
{
	Vector3			m_startPosition = Vector3::ZERO;
	Vector3			m_direction = Vector3::ZERO;
	float			m_maxDistance = 0.f;
	Vector3			m_endPosition = Vector3::ZERO;
	Vector3			m_impactPosition = Vector3::ZERO;
	float			m_impactFraction = 0.f;
	float			m_impactDistance = 0.f;
	BlockLocator	m_impactBlock = BlockLocator(nullptr, 0);
	Vector3			m_impactNormal = Vector3::ZERO;

	bool DidImpact() const { return m_impactFraction < 1.f; }
};

class Chunk;

class World
{
public:
	//-----Public Methods-----

	World();
	~World();

	void ProcessInput();
	void Update();
	void Render() const;

	IntVector2		GetChunkCoordsForChunkThatContainsPosition(const Vector2& position) const;
	IntVector2		GetChunkCoordsForChunkThatContainsPosition(const Vector3& position) const;
	Chunk*			GetChunkThatContainsPosition(const Vector2& position) const;
	Chunk*			GetChunkThatContainsPosition(const Vector3& position) const;
	Chunk*			GetChunkThatContainsFlooredPosition(const IntVector3& flooredPosition) const;
	BlockLocator	GetBlockLocatorThatContainsWorldPosition(const Vector3& position) const;
	BlockLocator	GetBlockLocatorForFlooredPosition(const IntVector3& coords) const;
	int				GetActiveChunkCount() const;
	float			GetTimeInDays() const;
	float			GetTimeOfDayZeroToOne() const;
	int				GetCurrentDayNumber() const;

	RaycastResult_t RaycastWithStepAndSample(const Vector3& start, const Vector3& directionNormal, float maxDistance) const;
	RaycastResult_t RaycastWithIntercepts(const Vector3& start, const Vector3& directionNormal, float maxDistance) const;


private:
	//-----Private Methods-----


	// Chunk Activation
	void			CheckToActivateChunks();
	void			PopulateBlocksOnChunk(Chunk* chunkToPopulate);
	void			AddChunkToActiveList(Chunk* chunkToAdd);
	bool			GetClosestInactiveChunkCoordsToPlayerWithinActivationRange(IntVector2& out_closestInactiveChunkCoords) const;
	void			InitializeLightingForChunk(Chunk* chunk);

	// Chunk Deactivation
	void			CheckToDeactivateChunks();
	void			RemoveChunkFromActiveList(Chunk* chunkToRemove);
	void			DeactivateChunk(Chunk* chunk);
	Chunk*			GetFarthestActiveChunkToPlayerOutsideDeactivationRange() const;

	// Chunk MeshBuilding
	void			CheckToBuildChunkMesh();
	bool			GetClosestActiveChunkToPlayerWithDirtyMesh(IntVector2& out_closestActiveDirtyCoords) const;

	// Lighting
	void			AddBlockToDirtyLightingList(BlockLocator blockLocator);
	BlockLocator	RemoveFrontBlockFromDirtyLightingList();
	void			InitializeSkyBlocksForChunk(Chunk* chunk);
	void			InitializeLightSourceBlocksForChunk(Chunk* chunk);
	void			SetEdgeBlocksAsDirty(Chunk* chunk);
	void			RecalculateLightingForBlock(BlockLocator blockLocator);
	void			UndirtyAllBlocksInChunk(Chunk* chunk);
	void			CalculateSkyColor();
	void			CalculateIndoorLightColor();
	void			CalculateOutdoorLightColor();

	// Input-Related
	void			DigBlock(BlockLocator blockToDig);
	void			PlaceBlock(BlockLocator hitBlockLocator);

	// Update
	void			UpdateChunks();
	void			UpdateRaycast();
	void			UpdateLighting();
	void			UpdateTimeOfDay();

	// Render
	void			RenderChunks() const;


private:
	//-----Private Data-----
	
	std::map<IntVector2, Chunk*>	m_activeChunks;
	std::deque<BlockLocator>		m_dirtyLightingBlocks;

	Vector3							m_skyColor = Vector3(1.0f, 0.9f, 0.8f);
	Vector3							m_indoorLightColor = Vector3(1.0f, 1.0f, 0.f);
	Vector3							m_outdoorLightColor = Vector3(1.0f, 0.9f, 0.8f);
	float							m_timeInDays = 0.f; // Cumulative, so 5.27 is 27% into the 6th day
	float							m_currentTimeScale = DEFAULT_WORLD_DAY_TIME_SCALE;

	// For Debugging
	bool							m_raycastDetached = false;
	bool							m_useStepAndSampleRaycast = true;
	Vector3							m_raycastReferencePosition; // Will be camera position when not detached
	Vector3							m_raycastForward;			// Will be the camera forward when not detached
	RaycastResult_t					m_lastRaycastResult;

	uint8_t							m_rightClickBlockType = 5;
	uint8_t							m_ctrlRightClickBlockType = 7;

	// Static constants
	static constexpr int			SEA_LEVEL = 25;
	static constexpr int			BASE_ELEVATION = 30;
	static constexpr int			NOISE_MAX_DEVIATION_FROM_BASE_ELEVATION = 10;
	static constexpr int			RAYCAST_STEPS_PER_BLOCK = 100;
	static constexpr float			DEFAULT_RAYCAST_DISTANCE = 8.f;

	static constexpr float			DEFAULT_WORLD_DAY_TIME_SCALE = 200.f;
	static constexpr float			ONE_OVER_SECONDS_PER_DAY = 1.f / 86400.f;
	static const Vector3			WORLD_NOON_SKY_COLOR;
	static const Vector3			WORLD_NIGHT_SKY_COLOR;
	static const Vector3			WORLD_INDOOR_LIGHT_COLOR;

};
