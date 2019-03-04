/************************************************************************/
/* File: World.cpp
/* Author: Andrew Chase
/* Date: February 9th 2019
/* Description: Implementation of the World class
/************************************************************************/
#include "Game/Framework/Game.hpp"
#include "Game/Environment/World.hpp"
#include "Game/Environment/Chunk.hpp"
#include "Game/Environment/Chunk.inl"
#include "Game/Framework/GameCamera.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Environment/BlockLocator.inl"
#include "Engine/Core/File.hpp"
#include "Engine/Math/Matrix44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Rendering/Core/Renderer.hpp"
#include "Engine/Core/Utility/Blackboard.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"
#include "Engine/Core/Utility/ErrorWarningAssert.hpp"
#include "Engine/Core/DeveloperConsole/DevConsole.hpp"
#include "Engine/Rendering/DebugRendering/DebugRenderSystem.hpp"\

//-----------------------------------------------------------------------------------------------
// Constructor
//
World::World()
{
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
World::~World()
{
	// Delete all active chunks
	// Don't use DeactivateChunk() to avoid invalidating this iterator
	std::map<IntVector2, Chunk*>::iterator chunkItr = m_activeChunks.begin();

	for (chunkItr; chunkItr != m_activeChunks.end(); chunkItr++)
	{
		Chunk* chunk = chunkItr->second;

		if (chunk->ShouldWriteToFile())
		{
			chunk->WriteToFile();
		}

		delete chunk;
	}

	m_activeChunks.clear();
}


//-----------------------------------------------------------------------------------------------
// Process Input
//
void World::ProcessInput()
{
	InputSystem* input = InputSystem::GetInstance();
	Mouse& mouse = input->GetMouse();

	if (input->WasKeyJustPressed('R'))
	{
		m_raycastDetached = !m_raycastDetached;
	}

	if (m_lastRaycastResult.DidImpact())
	{
		BlockLocator hitBlockLocator = m_lastRaycastResult.m_impactBlock;

		if (hitBlockLocator.IsValid()) // Ensures we don't do anything outside of a chunk
		{
			// Dig a block
			if (mouse.WasButtonJustPressed(MOUSEBUTTON_LEFT))
			{
				DigBlock(hitBlockLocator);
			}

			// Place a block
			if (mouse.WasButtonJustPressed(MOUSEBUTTON_RIGHT))
			{
				BlockLocator blockBeingPlaced = hitBlockLocator.StepInCoordDirection(IntVector3(m_lastRaycastResult.m_impactNormal));
				Chunk* chunkContainingPlacedBlock = blockBeingPlaced.GetChunk();
				int indexOfPlacedBlock = blockBeingPlaced.GetBlockIndex();

				const BlockType* blockType = BlockType::GetTypeByIndex(m_blockTypeToPlace);
				if (input->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
				{
					blockType = BlockType::GetTypeByName("Glowstone");
				}

				chunkContainingPlacedBlock->SetBlockTypeAtBlockIndex(indexOfPlacedBlock, blockType);
				chunkContainingPlacedBlock->SetNeedsToBeSavedToDisk(true);
			}
		}
	}

	// For mass deactivation
	if (input->WasKeyJustPressed('U'))
	{
		// Delete all active chunks
		std::map<IntVector2, Chunk*>::iterator chunkItr = m_activeChunks.begin();

		for (chunkItr; chunkItr != m_activeChunks.end(); chunkItr++)
		{
			Chunk* chunk = chunkItr->second;

			if (chunk->ShouldWriteToFile())
			{
				chunk->WriteToFile();
			}

			delete chunk;
		}

		m_activeChunks.clear();
	}
}


//-----------------------------------------------------------------------------------------------
// Updates all chunks in the world, as well as the camera
//
void World::Update()
{
	Game::GetGameCamera()->Update();

	CheckToActivateChunks();	// Create chunks within activation range
	CheckToDeactivateChunks();	// Save and remove a chunk if outside deactivation range

	UpdateLighting();			// Fix bad lighting on blocks that are dirty
	CheckToBuildChunkMesh();	// Build a mesh if one needs to be rebuilt

	UpdateChunks();				// General Update for chunks
	UpdateRaycast();			// Update for debug raycast
}


//-----------------------------------------------------------------------------------------------
// Renders all chunks to the screen
//
void World::Render() const
{
	RenderChunks();

	Renderer* renderer = Renderer::GetInstance();
	Material* xRayMaterial = AssetDB::GetSharedMaterial("X_Ray");

	if (m_raycastDetached)
	{
		if (m_lastRaycastResult.DidImpact())
		{
			renderer->Draw3DLine(m_lastRaycastResult.m_startPosition, Rgba::RED, m_lastRaycastResult.m_impactPosition, Rgba::RED, 5.f);
			renderer->Draw3DLine(m_lastRaycastResult.m_impactPosition, Rgba::RED, m_lastRaycastResult.m_endPosition, Rgba::RED, 2.f);
			renderer->Draw3DLine(m_lastRaycastResult.m_impactPosition, Rgba::RED, m_lastRaycastResult.m_endPosition, Rgba::RED, 2.f, xRayMaterial);

			renderer->DrawPoint(m_lastRaycastResult.m_impactPosition, Rgba::RED, 0.1f);
			renderer->DrawPoint(m_lastRaycastResult.m_impactPosition, Rgba::RED, 0.1f, xRayMaterial);
		}
		else
		{
			renderer->Draw3DLine(m_lastRaycastResult.m_startPosition, Rgba::GREEN, m_lastRaycastResult.m_endPosition, Rgba::GREEN, 2.f);
		}
	}

	// Push a wire cube around the block if there was a hit
	if (m_lastRaycastResult.DidImpact() && m_lastRaycastResult.m_impactDistance > 2.0f) // Only draw if more than one block away to avoid blocking vision
	{
		float offSetMagnitude = 0.01f;
		Vector3 blockCenterPosition = m_lastRaycastResult.m_impactBlock.GetBlockCenterWorldPosition();

		renderer->SetGLLineWidth(2.0f);
		renderer->DrawWireCube(blockCenterPosition, Vector3::ONES + Vector3(offSetMagnitude), Rgba::PURPLE);
		renderer->SetGLLineWidth(1.0f);

		Vector3 quadCenterPosition = blockCenterPosition + (0.5f + offSetMagnitude) * m_lastRaycastResult.m_impactNormal;
		Vector2 quadDimensions = Vector2(1.0f);

		Vector3 referenceRight = Vector3(-m_lastRaycastResult.m_impactNormal.y, m_lastRaycastResult.m_impactNormal.x, 0.f);
		if (referenceRight == Vector3::ZERO)
		{
			referenceRight = Vector3::MINUS_Y_AXIS;
		}

		Vector3 normal = m_lastRaycastResult.m_impactNormal;
		Vector3 up = CrossProduct(normal, referenceRight);
		Vector3 right = CrossProduct(up, normal);

		renderer->Draw3DQuad(quadCenterPosition, quadDimensions, AABB2::UNIT_SQUARE_OFFCENTER, right, up, Rgba::WHITE, Vector2(0.5f), AssetDB::GetSharedMaterial("Default_Alpha"));
		renderer->Draw3DQuad(quadCenterPosition, quadDimensions, AABB2::UNIT_SQUARE_OFFCENTER, right, up, Rgba::WHITE, Vector2(0.5f), xRayMaterial);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the chunk coordinates for the chunk that contains the given position
//
IntVector2 World::GetChunkCoordsForChunkThatContainsPosition(const Vector2& position) const
{
	int x = Floor(position.x / (float)Chunk::CHUNK_DIMENSIONS_X);
	int y = Floor(position.y / (float)Chunk::CHUNK_DIMENSIONS_Y);

	return IntVector2(x, y);
}


//-----------------------------------------------------------------------------------------------
// Returns the chunk coordinates that contain the given position
// Does not check the Z bounds
//
IntVector2 World::GetChunkCoordsForChunkThatContainsPosition(const Vector3& position) const
{
	return GetChunkCoordsForChunkThatContainsPosition(position.xy());
}


//-----------------------------------------------------------------------------------------------
// Returns the chunk that contains the given position
//
Chunk* World::GetChunkThatContainsPosition(const Vector2& position) const
{
	IntVector2 chunkCoords = GetChunkCoordsForChunkThatContainsPosition(position);

	bool chunkExists = m_activeChunks.find(chunkCoords) != m_activeChunks.end();
	if (chunkExists)
	{
		return m_activeChunks.at(chunkCoords);
	}

	return nullptr;
}


//-----------------------------------------------------------------------------------------------
// Returns the chunk that contains the given position
//
Chunk* World::GetChunkThatContainsPosition(const Vector3& position) const
{
	if (position.z < 0.f || position.z >= (float)Chunk::CHUNK_DIMENSIONS_Z)
	{
		return nullptr;
	}

	return GetChunkThatContainsPosition(position.xy());
}


//-----------------------------------------------------------------------------------------------
// Returns the chunk that contains the given block coords, which are defined in world space
//
Chunk* World::GetChunkThatContainsFlooredPosition(const IntVector3& flooredPosition) const
{
	if (flooredPosition.z < 0 || flooredPosition.z >= Chunk::CHUNK_DIMENSIONS_Z)
	{
		return nullptr;
	}

	// Get the chunk coords
	int chunkX = Floor((float)flooredPosition.x / (float)Chunk::CHUNK_DIMENSIONS_X);
	int chunkY = Floor((float)flooredPosition.y / (float)Chunk::CHUNK_DIMENSIONS_Y);

	IntVector2 chunkCoords = IntVector2(chunkX, chunkY);
	
	bool chunkExists = m_activeChunks.find(chunkCoords) != m_activeChunks.end();

	if (chunkExists)
	{
		return m_activeChunks.at(chunkCoords);
	}

	return nullptr;
}


//-----------------------------------------------------------------------------------------------
// Returns the block locator that points to the block containing the given position
//
BlockLocator World::GetBlockLocatorThatContainsWorldPosition(const Vector3& position) const
{
	Chunk* containingChunk = GetChunkThatContainsPosition(position);

	if (containingChunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}
	else
	{
		return containingChunk->GetBlockLocatorThatContainsPosition(position);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the block locator that points to the block that would contain the given floored position
// (i.e. a world position with all components floored to an int)
//
BlockLocator World::GetBlockLocatorForFlooredPosition(const IntVector3& flooredPosition) const
{
	Chunk* chunk = GetChunkThatContainsFlooredPosition(flooredPosition);
	
	if (chunk == nullptr)
	{
		return BlockLocator(nullptr, 0);
	}

	IntVector3 chunkOffset = IntVector3(chunk->GetOriginWorldPosition());
	IntVector3 blockCoords = flooredPosition - chunkOffset;

	int blockIndex = chunk->GetBlockIndexFromBlockCoords(blockCoords);

	return BlockLocator(chunk, blockIndex);
}


//-----------------------------------------------------------------------------------------------
// Returns the number of chunks currently active in the world
//
int World::GetActiveChunkCount() const
{
	return (int) m_activeChunks.size();
}


//-----------------------------------------------------------------------------------------------
// Performs a raycast from the given start in the direction, stopping after an impact or at maxDistance,
// whichever comes first
//
RaycastResult_t World::Raycast(const Vector3& start, const Vector3& directionNormal, float maxDistance) const
{
	// Check if we're raycasting from inside a solid block - if so immediately return

	int totalSteps = (int) (maxDistance * (float)RAYCAST_STEPS_PER_BLOCK);
	float stepSize = (1.f / (float)RAYCAST_STEPS_PER_BLOCK);
	
	IntVector3 lastPositionFloored = FloorPositionToIntegerCoords(start);

	for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex)
	{
		float distanceTravelled = (stepSize * (float)stepIndex);
		Vector3 currPos = start + (distanceTravelled * directionNormal);
		IntVector3 currPositionFloored = FloorPositionToIntegerCoords(currPos);

		// Don't do anything if we haven't moved into a new block
		if (lastPositionFloored == currPositionFloored)
		{
			continue;
		}

		// We're in a new block - step east/west, then north/south, then up/down to avoid corner tunneling
		IntVector3 flooredDiff = currPositionFloored - lastPositionFloored;
		if (flooredDiff.x != 0)
		{
			lastPositionFloored += IntVector3(flooredDiff.x, 0, 0);
			BlockLocator xStepLocator = GetBlockLocatorForFlooredPosition(lastPositionFloored);
			Block& block = xStepLocator.GetBlock();

			if (block.IsSolid())
			{
				RaycastResult_t impactResult;

				impactResult.m_startPosition = start;
				impactResult.m_direction = directionNormal;
				impactResult.m_maxDistance = maxDistance;
				impactResult.m_endPosition = start + (directionNormal * maxDistance);
				impactResult.m_impactPosition = currPos;
				impactResult.m_impactFraction = (distanceTravelled) / maxDistance;
				impactResult.m_impactDistance = distanceTravelled;
				impactResult.m_impactBlock = xStepLocator;
				impactResult.m_impactNormal = Vector3(-flooredDiff.x, 0, 0);

				return impactResult;
			}
		}
		
		if (flooredDiff.y != 0)
		{
			lastPositionFloored += IntVector3(0, flooredDiff.y, 0);
			BlockLocator yStepLocator = GetBlockLocatorForFlooredPosition(lastPositionFloored);
			Block& block = yStepLocator.GetBlock();

			if (block.IsSolid())
			{
				RaycastResult_t impactResult;

				impactResult.m_startPosition = start;
				impactResult.m_direction = directionNormal;
				impactResult.m_maxDistance = maxDistance;
				impactResult.m_endPosition = start + (directionNormal * maxDistance);
				impactResult.m_impactPosition = currPos;
				impactResult.m_impactFraction = (distanceTravelled) / maxDistance;
				impactResult.m_impactDistance = distanceTravelled;
				impactResult.m_impactBlock = yStepLocator;
				impactResult.m_impactNormal = Vector3(0, -flooredDiff.y, 0);

				return impactResult;
			}
		}

		if (flooredDiff.z != 0)
		{
			lastPositionFloored += IntVector3(0, 0, flooredDiff.z);
			BlockLocator zStepLocator = GetBlockLocatorForFlooredPosition(lastPositionFloored);
			Block& block = zStepLocator.GetBlock();

			if (block.IsSolid())
			{
				RaycastResult_t impactResult;

				impactResult.m_startPosition = start;
				impactResult.m_direction = directionNormal;
				impactResult.m_maxDistance = maxDistance;
				impactResult.m_endPosition = start + (directionNormal * maxDistance);
				impactResult.m_impactPosition = currPos;
				impactResult.m_impactFraction = (distanceTravelled) / maxDistance;
				impactResult.m_impactDistance = distanceTravelled;
				impactResult.m_impactBlock = zStepLocator;
				impactResult.m_impactNormal = Vector3(0, 0, -flooredDiff.z);

				return impactResult;
			}
		}

		// Update the last coords we were in
		lastPositionFloored = currPositionFloored;
	}

	// No impact, so return a negative result
	RaycastResult_t noHitResult;

	noHitResult.m_startPosition = start;
	noHitResult.m_direction = directionNormal;
	noHitResult.m_maxDistance = maxDistance;
	noHitResult.m_endPosition = start + (directionNormal * maxDistance);
	noHitResult.m_impactPosition = noHitResult.m_endPosition;
	noHitResult.m_impactFraction = 1.0f;
	noHitResult.m_impactDistance = maxDistance;
	noHitResult.m_impactBlock = GetBlockLocatorThatContainsWorldPosition(noHitResult.m_endPosition);
	noHitResult.m_impactNormal = -1.0f * directionNormal;

	return noHitResult;
}


//-----------------------------------------------------------------------------------------------
// Loads or generates a chunk at the given coords and adds it to the world
//
void World::PopulateBlocksOnChunk(Chunk* chunkToPopulate)
{
	// If the file for a chunk exists, load it
	IntVector2 chunkCoords = chunkToPopulate->GetChunkCoords();

	std::string filename = Stringf("Saves/Chunk_%i,%i.chunk", chunkCoords.x, chunkCoords.y);
	bool fromFileSuccess = chunkToPopulate->InitializeFromFile(filename);

	if (fromFileSuccess)
	{
		ConsolePrintf(Rgba::GREEN, "Chunk (%i, %i) activated from file", chunkCoords.x, chunkCoords.y);
	}
	else
	{
		chunkToPopulate->GenerateWithPerlinNoise(BASE_ELEVATION, NOISE_MAX_DEVIATION_FROM_BASE_ELEVATION, SEA_LEVEL);
		ConsolePrintf(Rgba::GREEN, "Chunk (%i, %i) generated from noise", chunkCoords.x, chunkCoords.y);
	}
}


//-----------------------------------------------------------------------------------------------
// Removes the chunk from the world's list of active chunks, and writes to file if it has been modified
// DELETES THE CHUNK
//
void World::DeactivateChunk(Chunk* chunk)
{
	// Write to file if needed
	if (chunk->ShouldWriteToFile())
	{
		chunk->WriteToFile();
	}

	delete chunk;
}


//-----------------------------------------------------------------------------------------------
// Checks if there is an inactive chunk within the activation range, and returns true if there exists
// one, returning the closest one
//
bool World::GetClosestInactiveChunkCoordsToPlayerWithinActivationRange(IntVector2& out_closestInactiveChunkCoords) const
{
	Blackboard* config = Game::GetGameConfigBlackboard();
	float activationRange = config->GetValue("activation_range", DEFAULT_CHUNK_ACTIVATION_RANGE);
	float activationRangeSquared = activationRange * activationRange;

	int chunkSpanX = Ceiling(activationRange / (float)Chunk::CHUNK_DIMENSIONS_X);
	int chunkSpanY = Ceiling(activationRange / (float)Chunk::CHUNK_DIMENSIONS_Y);
	IntVector2 chunkSpan = IntVector2(chunkSpanX, chunkSpanY);

	Vector2 cameraXYPosition = Game::GetGameCamera()->GetPosition().xy();
	IntVector2 chunkContainingCamera = GetChunkCoordsForChunkThatContainsPosition(cameraXYPosition);

	IntVector2 startChunk = chunkContainingCamera - chunkSpan;
	IntVector2 endChunk = chunkContainingCamera + chunkSpan;

	float minDistanceSoFar = activationRangeSquared;
	bool foundInactiveChunk = false;

	for (int y = startChunk.y; y <= endChunk.y; ++y)
	{
		for (int x = startChunk.x; x <= endChunk.x; ++x)
		{
			IntVector2 currChunkCoords = IntVector2(x, y);

			// If the chunk is already active just continue
			if (m_activeChunks.find(currChunkCoords) != m_activeChunks.end())
			{
				continue;
			}

			// Get the distance to the chunk
			Vector2 chunkBasePosition = Vector2(currChunkCoords.x * Chunk::CHUNK_DIMENSIONS_X, currChunkCoords.y * Chunk::CHUNK_DIMENSIONS_Y);
			Vector2 chunkXYCenter = chunkBasePosition + 0.5f * Vector2(Chunk::CHUNK_DIMENSIONS_X, Chunk::CHUNK_DIMENSIONS_Y);
			Vector2 vectorToChunkCenter = (chunkXYCenter - cameraXYPosition);

			float distanceSquared = vectorToChunkCenter.GetLengthSquared();
			
			// Update our min if it's smaller
			if (distanceSquared < minDistanceSoFar)
			{
				minDistanceSoFar = distanceSquared;
				out_closestInactiveChunkCoords = currChunkCoords;
				foundInactiveChunk = true;
			}
		}
	}

	return foundInactiveChunk;
}


//-----------------------------------------------------------------------------------------------
// Sets up the initial lighting values for the chunk so that the lighting algorithm can run on it
//
void World::InitializeLightingForChunk(Chunk* chunk)
{
	InitializeSkyBlocksForChunk(chunk);
	InitializeLightSourceBlocksForChunk(chunk);
	SetNeighborEdgeBlocksToDirtyForChunk(chunk);
}


//-----------------------------------------------------------------------------------------------
// Returns true if it finds an active chunk outside the deactivation range, and returns the coords
// of the closest one
//
Chunk* World::GetFarthestActiveChunkToPlayerOutsideDeactivationRange() const
{
	std::map<IntVector2, Chunk*>::const_iterator itr = m_activeChunks.begin();

	Vector2 cameraXYPosition = Game::GetGameCamera()->GetPosition().xy();

	
	Blackboard* config = Game::GetGameConfigBlackboard();
	float activationRange = config->GetValue("activation_range", DEFAULT_CHUNK_ACTIVATION_RANGE);
	float deactivationOffset = config->GetValue("deactivation_offset", DEFAULT_CHUNK_DEACTIVATION_OFFSET);

	float deactivationRangeSquared = activationRange + deactivationOffset;
	deactivationRangeSquared *= deactivationRangeSquared;

	float maxDistance = 0.f;
	Chunk* farthestActiveChunkOutsideDeactivationRange = nullptr;
	
	for (itr; itr != m_activeChunks.end(); itr++)
	{
		Chunk* currChunk = itr->second;

		Vector2 chunkXYCenter = currChunk->GetWorldXYCenter();
		Vector2 vectorToChunk = (chunkXYCenter - cameraXYPosition);
		float distanceSquared = vectorToChunk.GetLengthSquared();

		if (distanceSquared > deactivationRangeSquared && distanceSquared > maxDistance)
		{
			maxDistance = distanceSquared;
			farthestActiveChunkOutsideDeactivationRange = currChunk;
		}
	}

	return farthestActiveChunkOutsideDeactivationRange;
}


//-----------------------------------------------------------------------------------------------
// Checks for a chunk that needs its mesh rebuilt, and rebuilds it if found
//
void World::CheckToBuildChunkMesh()
{
	IntVector2 closestDirtyCoords;
	bool dirtyMeshFound = GetClosestActiveChunkToPlayerWithDirtyMesh(closestDirtyCoords);

	if (dirtyMeshFound)
	{
		Chunk* dirtyMeshChunk = m_activeChunks[closestDirtyCoords];
		dirtyMeshChunk->BuildMesh();
	}
}


//-----------------------------------------------------------------------------------------------
// Searches for a chunk in the active list that has a dirty mesh, and returns the closest one
//
bool World::GetClosestActiveChunkToPlayerWithDirtyMesh(IntVector2& out_closestActiveDirtyCoords) const
{
	std::map<IntVector2, Chunk*>::const_iterator itr = m_activeChunks.begin();

	Vector2 cameraXYPosition = Game::GetGameCamera()->GetPosition().xy();

	float minDistance = 0.f;
	bool foundDirtyChunk = false;

	for (itr; itr != m_activeChunks.end(); itr++)
	{
		Chunk* currChunk = itr->second;

		if (!currChunk->IsMeshDirty() || !currChunk->HasAllFourNeighbors())
		{
			continue;
		}

		// Get the distance to the dirty chunk
		Vector2 chunkXYCenter = currChunk->GetWorldXYCenter();

		Vector2 vectorToChunk = (chunkXYCenter - cameraXYPosition);
		float distanceSquared = vectorToChunk.GetLengthSquared();

		if (!foundDirtyChunk || distanceSquared < minDistance)
		{
			minDistance = distanceSquared;
			out_closestActiveDirtyCoords = currChunk->GetChunkCoords();
			foundDirtyChunk = true;
		}
	}

	return foundDirtyChunk;
}


//-----------------------------------------------------------------------------------------------
// Adds the block to the dirty lighting list, checking for duplicates
//
void World::AddBlockToDirtyLightingList(BlockLocator blockLocator)
{
	Block& block = blockLocator.GetBlock();

	if (!block.IsInDirtyLightingList())
	{
		block.SetIsInDirtyLightingList(true);
		m_dirtyLightingBlocks.push_back(blockLocator);
	}
}


//-----------------------------------------------------------------------------------------------
// Removes the block from the front of the dirty lighting list, and unflags it as being part
// of the list
//
BlockLocator World::RemoveFrontBlockFromDirtyLightingList()
{
	ASSERT_OR_DIE(m_dirtyLightingBlocks.size() > 0, "Dirty list was empty!");

	BlockLocator blockLocator = m_dirtyLightingBlocks.front();
	m_dirtyLightingBlocks.pop_front();
	blockLocator.GetBlock().SetIsInDirtyLightingList(false);

	return blockLocator;
}


//-----------------------------------------------------------------------------------------------
// Finds and sets all blocks considered sky in the block, and sets adjacent neighbors
// to dirty to begin outdoor light cascading
//
void World::InitializeSkyBlocksForChunk(Chunk* chunk)
{
// PASS 1 - Find all sky blocks and set their lighting to 15, flag them as sky
// Sky == "I am not opaque, and no one above me is opaque"
for (int yIndex = 0; yIndex < Chunk::CHUNK_DIMENSIONS_Y; ++yIndex)
{
	for (int xIndex = 0; xIndex < Chunk::CHUNK_DIMENSIONS_X; ++xIndex)
	{
		for (int zIndex = Chunk::CHUNK_DIMENSIONS_Z - 1; zIndex >= 0; --zIndex)
		{
			IntVector3 blockCoords = IntVector3(xIndex, yIndex, zIndex);
			Block& block = chunk->GetBlock(blockCoords);

			if (!block.IsFullyOpaque())
			{
				block.SetIsPartOfSky(true);
				block.SetOutdoorLighting(Block::BLOCK_MAX_LIGHTING);
			}
			else
			{
				break;
			}
		}
	}
}

// Pass 2 - Set horizontal neighbors to sky blocks as dirty to propogate the light
for (int blockIndex = 0; blockIndex < Chunk::BLOCKS_PER_CHUNK; ++blockIndex)
{
	BlockLocator blockLocator(chunk, blockIndex); // simple constructions
	Block& block = blockLocator.GetBlock();

	if (!block.IsPartOfSky())
	{
		continue;
	}

	BlockLocator eastLocator = blockLocator.ToEast();
	BlockLocator westLocator = blockLocator.ToWest();
	BlockLocator northLocator = blockLocator.ToNorth();
	BlockLocator southLocator = blockLocator.ToSouth();

	// East
	if (eastLocator.IsValid())
	{
		Block& eastBlock = eastLocator.GetBlock();

		if (!eastBlock.IsFullyOpaque() && !eastBlock.IsPartOfSky())
		{
			AddBlockToDirtyLightingList(eastLocator);
		}
	}

	// West
	if (westLocator.IsValid())
	{
		Block& westBlock = westLocator.GetBlock();

		if (!westBlock.IsFullyOpaque() && !westBlock.IsPartOfSky())
		{
			AddBlockToDirtyLightingList(westLocator);
		}
	}

	// North
	if (northLocator.IsValid())
	{
		Block& northBlock = northLocator.GetBlock();

		if (!northBlock.IsFullyOpaque() && !northBlock.IsPartOfSky())
		{
			AddBlockToDirtyLightingList(northLocator);
		}
	}

	// South
	if (southLocator.IsValid())
	{
		Block& southBlock = southLocator.GetBlock();

		if (!southBlock.IsFullyOpaque() && !southBlock.IsPartOfSky())
		{
			AddBlockToDirtyLightingList(southLocator);
		}
	}
}
}


//-----------------------------------------------------------------------------------------------
// Searches the chunk for light sources and flags them for dirty so they propagate
//
void World::InitializeLightSourceBlocksForChunk(Chunk* chunk)
{
	for (int blockIndex = 0; blockIndex < Chunk::BLOCKS_PER_CHUNK; ++blockIndex)
	{
		BlockLocator blockLocator(chunk, blockIndex);
		Block& block = blockLocator.GetBlock();
		const BlockType* blockType = block.GetType();

		if (!block.IsFullyOpaque() && blockType->m_internalLightLevel > 0)
		{
			// Do not set the lighting of this block! Let the algorithm set it
			AddBlockToDirtyLightingList(blockLocator);
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Sets the non-opaque edge blocks of neighbors to be dirty so light will propogate into them
// when this chunk is fully initialized
//
void World::SetNeighborEdgeBlocksToDirtyForChunk(Chunk* chunk)
{
	Chunk* eastChunk = chunk->GetEastNeighbor();
	Chunk* westChunk = chunk->GetWestNeighbor();
	Chunk* northChunk = chunk->GetNorthNeighbor();
	Chunk* southChunk = chunk->GetSouthNeighbor();

	// East
	if (eastChunk != nullptr)
	{
		for (int zIndex = 0; zIndex < Chunk::CHUNK_DIMENSIONS_Z; ++zIndex)
		{
			for (int yIndex = 0; yIndex < Chunk::CHUNK_DIMENSIONS_Y; ++yIndex)
			{
				IntVector3 blockCoords = IntVector3(0, yIndex, zIndex);
				BlockLocator blockLocator(eastChunk, blockCoords);
				Block& block = blockLocator.GetBlock();

				if (!block.IsFullyOpaque())
				{
					AddBlockToDirtyLightingList(blockLocator);
				}
			}
		}
	}

	// West
	if (westChunk != nullptr)
	{
		for (int zIndex = 0; zIndex < Chunk::CHUNK_DIMENSIONS_Z; ++zIndex)
		{
			for (int yIndex = 0; yIndex < Chunk::CHUNK_DIMENSIONS_Y; ++yIndex)
			{
				IntVector3 blockCoords = IntVector3(Chunk::CHUNK_DIMENSIONS_X - 1, yIndex, zIndex);
				BlockLocator blockLocator(westChunk, blockCoords);
				Block& block = blockLocator.GetBlock();

				if (!block.IsFullyOpaque())
				{
					AddBlockToDirtyLightingList(blockLocator);
				}
			}
		}
	}

	// North
	if (northChunk != nullptr)
	{
		for (int zIndex = 0; zIndex < Chunk::CHUNK_DIMENSIONS_Z; ++zIndex)
		{
			for (int xIndex = 0; xIndex < Chunk::CHUNK_DIMENSIONS_X; ++xIndex)
			{
				IntVector3 blockCoords = IntVector3(xIndex, 0, zIndex);
				BlockLocator blockLocator(northChunk, blockCoords);
				Block& block = blockLocator.GetBlock();

				if (!block.IsFullyOpaque())
				{
					AddBlockToDirtyLightingList(blockLocator);
				}
			}
		}
	}

	// South
	if (southChunk != nullptr)
	{
		for (int zIndex = 0; zIndex < Chunk::CHUNK_DIMENSIONS_Z; ++zIndex)
		{
			for (int xIndex = 0; xIndex < Chunk::CHUNK_DIMENSIONS_X; ++xIndex)
			{
				IntVector3 blockCoords = IntVector3(xIndex, 0, zIndex);
				BlockLocator blockLocator(southChunk, blockCoords);
				Block& block = blockLocator.GetBlock();

				if (!block.IsFullyOpaque())
				{
					AddBlockToDirtyLightingList(blockLocator);
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Recalculates and sets what this block's light value should be, and if there is a change
// pushes all neighboring block in the dirty list to also be recalculated
//
void World::RecalculateLightingForBlock(BlockLocator blockLocator)
{
	BlockLocator eastBlockLocator = blockLocator.ToEast();
	BlockLocator westBlockLocator = blockLocator.ToWest();
	BlockLocator northBlockLocator = blockLocator.ToNorth();
	BlockLocator southBlockLocator = blockLocator.ToSouth();
	BlockLocator aboveBlockLocator = blockLocator.ToAbove();
	BlockLocator belowBlockLocator = blockLocator.ToBelow();

	Block& currBlock = blockLocator.GetBlock();
	Block& eastBlock = eastBlockLocator.GetBlock();
	Block& westBlock = westBlockLocator.GetBlock();
	Block& northBlock = northBlockLocator.GetBlock();
	Block& southBlock = southBlockLocator.GetBlock();
	Block& aboveBlock = aboveBlockLocator.GetBlock();
	Block& belowBlock = belowBlockLocator.GetBlock();

	int currIndoorLight = currBlock.GetIndoorLight();
	int currInternalLight = currBlock.GetType()->m_internalLightLevel;
	int eastIndoorLight = eastBlock.GetIndoorLight();
	int westIndoorLight = westBlock.GetIndoorLight();
	int northIndoorLight = northBlock.GetIndoorLight();
	int southIndoorLight = southBlock.GetIndoorLight();
	int aboveIndoorLight = aboveBlock.GetIndoorLight();
	int belowIndoorLight = belowBlock.GetIndoorLight();

	int currOutdoorLight = currBlock.GetOutdoorLight();
	int eastOutdoorLight = eastBlock.GetOutdoorLight();
	int westOutdoorLight = westBlock.GetOutdoorLight();
	int northOutdoorLight = northBlock.GetOutdoorLight();
	int southOutdoorLight = southBlock.GetOutdoorLight();
	int aboveOutdoorLight = aboveBlock.GetOutdoorLight();
	int belowOutdoorLight = belowBlock.GetOutdoorLight();

	int maxNeighborIndoorLight = Max(eastIndoorLight, westIndoorLight, northIndoorLight, southIndoorLight, aboveIndoorLight, belowIndoorLight);
	int maxNeighborOutdoorLight = Max(eastOutdoorLight, westOutdoorLight, northOutdoorLight, southOutdoorLight, aboveOutdoorLight, belowOutdoorLight);

	int expectedIndoorLight = currInternalLight;
	if (!currBlock.IsFullyOpaque())
	{
		expectedIndoorLight = Max(maxNeighborIndoorLight - 1, currInternalLight);
	}
	
	int expectedOutdoorLight = 0;
	if (currBlock.IsPartOfSky())
	{
		expectedOutdoorLight = Block::BLOCK_MAX_LIGHTING;
	}
	else if (!currBlock.IsFullyOpaque())
	{
		expectedOutdoorLight = ClampInt(maxNeighborOutdoorLight - 1, 0, Block::BLOCK_MAX_LIGHTING);
	}

	bool currLightValuesWrong = (expectedIndoorLight != currIndoorLight || expectedOutdoorLight != currOutdoorLight);

	if (currLightValuesWrong)
	{
		// Fix them
		currBlock.SetIndoorLighting(expectedIndoorLight);
		currBlock.SetOutdoorLighting(expectedOutdoorLight);;

		// Dirty all neighbors
 		AddBlockToDirtyLightingList(eastBlockLocator);
 		AddBlockToDirtyLightingList(westBlockLocator);
 		AddBlockToDirtyLightingList(northBlockLocator);
 		AddBlockToDirtyLightingList(southBlockLocator);
 		AddBlockToDirtyLightingList(aboveBlockLocator);
 		AddBlockToDirtyLightingList(belowBlockLocator);
	}
}


//-----------------------------------------------------------------------------------------------
// Digs a block by setting the block we're looking at to air
//
void World::DigBlock(BlockLocator blockToDig)
{
	int indexOfHitBlock = blockToDig.GetBlockIndex();
	Chunk* chunkContainingHit = blockToDig.GetChunk();

	const BlockType* blockType = BlockType::GetTypeByIndex(BlockType::AIR_TYPE_INDEX);
	chunkContainingHit->SetBlockTypeAtBlockIndex(indexOfHitBlock, blockType);
	chunkContainingHit->SetNeedsToBeSavedToDisk(true);
	
	AddBlockToDirtyLightingList(blockToDig);
}


//-----------------------------------------------------------------------------------------------
// Calls Update() on all chunks
//
void World::UpdateChunks()
{
	std::map<IntVector2, Chunk*>::iterator chunkItr = m_activeChunks.begin();

	for (chunkItr; chunkItr != m_activeChunks.end(); chunkItr++)
	{
		chunkItr->second->Update();
	}
}


//-----------------------------------------------------------------------------------------------
// Updates the last raycast struct stored on world, for debugging
//
void World::UpdateRaycast()
{
	GameCamera* camera = Game::GetGameCamera();

	// Update the raycast start if it isn't detached
	if (!m_raycastDetached)
	{
		m_raycastReferencePosition = camera->GetPosition();
		m_raycastForward = camera->GetCameraMatrix().GetIVector().xyz();
	}

	m_lastRaycastResult = Raycast(m_raycastReferencePosition, m_raycastForward, DEFAULT_RAYCAST_DISTANCE);
}


//-----------------------------------------------------------------------------------------------
// Calculates all lighting changes for all blocks in the dirty block queue
//
void World::UpdateLighting()
{
	while (m_dirtyLightingBlocks.size() > 0)
	{
		BlockLocator blockLocator = RemoveFrontBlockFromDirtyLightingList();
		RecalculateLightingForBlock(blockLocator);
	}
}


//-----------------------------------------------------------------------------------------------
// Calls Render() on all the chunk
//
void World::RenderChunks() const
{
	std::map<IntVector2, Chunk*>::const_iterator chunkItr = m_activeChunks.begin();

	for (chunkItr; chunkItr != m_activeChunks.end(); chunkItr++)
	{
		chunkItr->second->Render();
	}
}


//-----------------------------------------------------------------------------------------------
// Adds the chunk to the world map, and connects any references the chunk has to its neighbors
//
void World::AddChunkToActiveList(Chunk* chunkToAdd)
{
	// Check for duplicates
	IntVector2 chunkCoords = chunkToAdd->GetChunkCoords();
	bool alreadyExists = m_activeChunks.find(chunkCoords) != m_activeChunks.end();
	ASSERT_OR_DIE(!alreadyExists, Stringf("World attempted to add duplicate chunk at coords (%i, %i)", chunkCoords.x, chunkCoords.y).c_str());

	// Add it to the map
	m_activeChunks[chunkCoords] = chunkToAdd;

	// Hook up the references to the neighbors
	IntVector2 eastCoords = chunkCoords + IntVector2(1, 0);
	IntVector2 westCoords = chunkCoords + IntVector2(-1, 0);
	IntVector2 northCoords = chunkCoords + IntVector2(0, 1);
	IntVector2 southCoords = chunkCoords + IntVector2(0, -1);

	bool eastExists = m_activeChunks.find(eastCoords) != m_activeChunks.end();
	bool westExists = m_activeChunks.find(westCoords) != m_activeChunks.end();
	bool northExists = m_activeChunks.find(northCoords) != m_activeChunks.end();
	bool southExists = m_activeChunks.find(southCoords) != m_activeChunks.end();

	if (eastExists)
	{
		Chunk* eastChunk = m_activeChunks[eastCoords];

		chunkToAdd->SetEastNeighbor(eastChunk);
		eastChunk->SetWestNeighbor(chunkToAdd);
	}

	if (westExists)
	{
		Chunk* westChunk = m_activeChunks[westCoords];

		chunkToAdd->SetWestNeighbor(westChunk);
		westChunk->SetEastNeighbor(chunkToAdd);
	}

	if (northExists)
	{
		Chunk* northChunk = m_activeChunks[northCoords];

		chunkToAdd->SetNorthNeighbor(northChunk);
		northChunk->SetSouthNeighbor(chunkToAdd);
	}

	if (southExists)
	{
		Chunk* southChunk = m_activeChunks[southCoords];

		chunkToAdd->SetSouthNeighbor(southChunk);
		southChunk->SetNorthNeighbor(chunkToAdd);
	}
}


//-----------------------------------------------------------------------------------------------
// Removes this chunk from the active list in the world and unhooks any references to neighbors
//
void World::RemoveChunkFromActiveList(Chunk* chunkToRemove)
{
	IntVector2 chunkCoords = chunkToRemove->GetChunkCoords();
	bool chunkExists = m_activeChunks.find(chunkCoords) != m_activeChunks.end();
	ASSERT_OR_DIE(chunkExists, "World tried to remove chunk that doesn't exist");

	m_activeChunks.erase(chunkCoords);

	// Remove the connections
	Chunk* eastNeighbor = chunkToRemove->GetEastNeighbor();
	Chunk* westNeighbor = chunkToRemove->GetWestNeighbor();
	Chunk* northNeighbor = chunkToRemove->GetNorthNeighbor();
	Chunk* southNeighbor = chunkToRemove->GetSouthNeighbor();

	if (eastNeighbor != nullptr)
	{
		eastNeighbor->SetWestNeighbor(nullptr);
	}

	if (westNeighbor != nullptr)
	{
		westNeighbor->SetEastNeighbor(nullptr);
	}

	if (northNeighbor != nullptr)
	{
		northNeighbor->SetSouthNeighbor(nullptr);
	}

	if (southNeighbor != nullptr)
	{
		southNeighbor->SetNorthNeighbor(nullptr);
	}

	chunkToRemove->SetEastNeighbor(nullptr);
	chunkToRemove->SetWestNeighbor(nullptr);
	chunkToRemove->SetNorthNeighbor(nullptr);
	chunkToRemove->SetSouthNeighbor(nullptr);
}


//-----------------------------------------------------------------------------------------------
// Checks if there is a chunk within the activation range that is inactive, and activates it
//
void World::CheckToActivateChunks()
{
	IntVector2 closestInactiveChunkCoords;
	bool foundInactiveChunk = GetClosestInactiveChunkCoordsToPlayerWithinActivationRange(closestInactiveChunkCoords);

	if (foundInactiveChunk)
	{
		ConsolePrintf("Activating Chunk (%i, %i)", closestInactiveChunkCoords.x, closestInactiveChunkCoords.y);

		// Populate from data or noise
		Chunk* chunk = new Chunk(closestInactiveChunkCoords);
		PopulateBlocksOnChunk(chunk);

		// Add to the list
		AddChunkToActiveList(chunk);

		// Initialize lighting after adding so it can dirty its neighbor's blocks 
		InitializeLightingForChunk(chunk);
	}
}


//-----------------------------------------------------------------------------------------------
// Checks if any active chunks are outside the deactivation range, and if so removes them
//
void World::CheckToDeactivateChunks()
{
	Chunk* chunkToDeactivate = GetFarthestActiveChunkToPlayerOutsideDeactivationRange();

	if (chunkToDeactivate != nullptr)
	{
		IntVector2 chunkCoords = chunkToDeactivate->GetChunkCoords();
		ConsolePrintf(Rgba::ORANGE, "Deactivating Chunk (%i, %i)", chunkCoords.x, chunkCoords.y);

		RemoveChunkFromActiveList(chunkToDeactivate);
		DeactivateChunk(chunkToDeactivate);
	}
}
