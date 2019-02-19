/************************************************************************/
/* File: BlockLocator.cpp
/* Author: Andrew Chase
/* Date: February 18th 2019
/* Description: Implementation of the BlockLocator class
/************************************************************************/
#include "Game/Environment/Chunk.hpp"
#include "Game/Environment/Block.hpp"
#include "Game/Environment/BlockLocator.hpp"


//-----------------------------------------------------------------------------------------------
// Constructor
//
BlockLocator::BlockLocator(Chunk* chunk, int blockIndex)
	: m_chunk(chunk), m_blockIndex(blockIndex)
{
}


//-----------------------------------------------------------------------------------------------
// Returns the block this locator references, or the missing block if it doesn't exist
//
Block& BlockLocator::GetBlock()
{
	if (m_chunk == nullptr)
	{
		return Block::MISSING_BLOCK;
	}
	else
	{
		return m_chunk->GetBlock(m_blockIndex);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the chunk that contains the block this locator locates
//
Chunk* BlockLocator::GetChunk()
{
	return m_chunk;
}


//-----------------------------------------------------------------------------------------------
// Returns a locator to the block that is east of this block
//
BlockLocator BlockLocator::ToEast() const
{
	// If we're at the east boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_X_MASK) == Chunk::CHUNK_X_MASK)
	{
		int newBlockIndex = m_blockIndex & ~Chunk::CHUNK_X_MASK;
		return BlockLocator(m_chunk->GetEastNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex + 1);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns a locator to the block that is west of this block
//
BlockLocator BlockLocator::ToWest() const
{
	// If we're at the west boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_X_MASK) == 0)
	{
		int newBlockIndex = m_blockIndex | Chunk::CHUNK_X_MASK;
		return BlockLocator(m_chunk->GetWestNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex - 1);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns a locator to the block that is north of this block
//
BlockLocator BlockLocator::ToNorth() const
{
	// If we're at the north boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_Y_MASK) == Chunk::CHUNK_Y_MASK)
	{
		int newBlockIndex = m_blockIndex & ~Chunk::CHUNK_Y_MASK;
		return BlockLocator(m_chunk->GetNorthNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex + Chunk::CHUNK_DIMENSIONS_X);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns a locator to the block that is south of this block
//
BlockLocator BlockLocator::ToSouth() const
{
	// If we're at the south boundary of this chunk, move into the next chunk
	if ((m_blockIndex & Chunk::CHUNK_Y_MASK) == 0)
	{
		int newBlockIndex = m_blockIndex | Chunk::CHUNK_Y_MASK;
		return BlockLocator(m_chunk->GetSouthNeighbor(), newBlockIndex);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex - Chunk::CHUNK_DIMENSIONS_X);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns a locator to the block that is above this block
//
BlockLocator BlockLocator::ToAbove() const
{
	// If we're at the top of the chunk return a missing block locator
	if ((m_blockIndex & Chunk::CHUNK_Z_MASK) == Chunk::CHUNK_Z_MASK)
	{
		return BlockLocator(nullptr, -1);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex + Chunk::BLOCKS_PER_Z_LAYER);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns a locater to the block that is below this block
//
BlockLocator BlockLocator::ToBelow() const
{
	// If we're at the bottom of the chunk return a missing block locator
	if ((m_blockIndex & Chunk::CHUNK_Z_MASK) == 0)
	{
		return BlockLocator(nullptr, -1);
	}
	else
	{
		return BlockLocator(m_chunk, m_blockIndex - Chunk::BLOCKS_PER_Z_LAYER);
	}
}
