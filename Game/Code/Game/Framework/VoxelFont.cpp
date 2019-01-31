/************************************************************************/
/* File: VoxelFont.cpp
/* Author: Andrew Chase
/* Date: November 6th 2018
/* Description: Implementation of the VoxelFont class
/************************************************************************/
#include "Game/Framework/Game.hpp"
#include "Game/Framework/World.hpp"
#include "Game/Framework/VoxelFont.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Core/Time/Clock.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"
#include "Engine/Core/Utility/ErrorWarningAssert.hpp"


//-----------------------------------------------------------------------------------------------
// Constructor
//
VoxelFont::VoxelFont(const std::string& name, const std::string& imageFile)
	: m_name(name)
	, m_glyphLayout(IntVector2(16, 16))
{
	m_image = AssetDB::CreateOrGetImage(imageFile);
	if (!m_image->IsFlippedForTextures())
	{
		m_image->FlipVertical();
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the color of the pixel for the given glyph with the offset from the bottom left of the glyph
//
Rgba VoxelFont::GetColorForGlyphPixel(const char glyph, const IntVector2& offset) const
{
	IntVector2 totalDimensions = m_image->GetTexelDimensions();
	IntVector2 spriteLayout = IntVector2(16, 16);
	int glyphIndex = (int)glyph;

	// Get the glyph bottom left
	IntVector2 spriteCoord;
	spriteCoord.x = (glyphIndex % spriteLayout.x);
	spriteCoord.y = (glyphIndex / spriteLayout.x);
	spriteCoord.y = spriteLayout.y - spriteCoord.y - 1;

	int spriteStepX = (totalDimensions.x / spriteLayout.x);
	int spriteStepY = (totalDimensions.y / spriteLayout.y);

	IntVector2 spriteBottomLeft = IntVector2(spriteCoord.x * spriteStepX, spriteCoord.y * spriteStepY);
	IntVector2 finalCoord = spriteBottomLeft + offset;

	return m_image->GetTexelColor(finalCoord.x, finalCoord.y);
}


//-----------------------------------------------------------------------------------------------
// Returns the dimensions of a single glyph
//
IntVector3 VoxelFont::GetGlyphDimensions() const
{
	IntVector2 spriteLayout = IntVector2(16, 16);
	IntVector2 imageDimensions = m_image->GetTexelDimensions();

	return IntVector3(imageDimensions.x / spriteLayout.x, imageDimensions.y / spriteLayout.y, 1);
}


//-----------------------------------------------------------------------------------------------
// Returns the 3D voxel dimensions of the given string if written out with this font
// Always sets the local z-dimension to 1
//
IntVector3 VoxelFont::GetTextDimensions(const std::string& text) const
{
	IntVector2 glyphDimensions = m_image->GetTexelDimensions() / 16;

	int numChars = (int)text.size();
	
	return IntVector3(numChars * glyphDimensions.x, glyphDimensions.y, 1);
}


//-----------------------------------------------------------------------------------------------
// Returns an offset to be applied to the current voxel at world coords when rendering fonts
//
IntVector3 GetOffsetForFontWaveEffect(const IntVector3& localCoords, const IntVector3& worldCoords, void* args)
{
	UNUSED(args);
	UNUSED(localCoords);

	int frontRange = 15;
	int rearRange = 50;
	float maxOffset = 10;

	IntVector3 worldDimensions = Game::GetWorld()->GetDimensions();
	int time = (int)(100.f * Game::GetGameClock()->GetTotalSeconds());

	int target = (time % (2 * worldDimensions.x)) - frontRange;
	int displacement = worldCoords.x - target;
	int distance = AbsoluteValue(displacement);

	IntVector3 offset = IntVector3::ZERO;

	if (displacement >= 0 && displacement <= frontRange)
	{
		float t = (float)(frontRange - distance) / (float)frontRange;
		offset.z = (int)(-maxOffset * t);
	}
	else if (displacement < 0 && displacement > -rearRange)
	{
		float t = (float)(rearRange - distance) / (float)(rearRange);
		offset.z = (int)(-maxOffset * t);
	}

	return offset;
}


//-----------------------------------------------------------------------------------------------
// Returns the color for the given voxel to make a white wave effect along the X axis
//
Rgba GetColorForWaveEffect(const IntVector3& localCoords, const IntVector3& worldCoords, const Rgba& baseColor, void* args)
{
	UNUSED(localCoords);

	// Get the args
	VoxelFontColorWaveArgs_t colorArgs = *(VoxelFontColorWaveArgs_t*)args;

	int frontRange = 15;
	int rearRange = 50;

	IntVector3 worldDimensions = Game::GetWorld()->GetDimensions();

	int axisLength = (colorArgs.direction.y == 0 ? worldDimensions.x : worldDimensions.y);
	int coordAlongAxis = worldCoords.x;
	int directionCoefficient = colorArgs.direction.x;

	if (colorArgs.direction.y != 0)
	{
		coordAlongAxis = worldCoords.y;
		directionCoefficient = colorArgs.direction.y;
	}
	else if (colorArgs.direction.z != 0)
	{
		coordAlongAxis = worldCoords.z;
		directionCoefficient = colorArgs.direction.z;
	}

	int time = (int) (colorArgs.speed * (float)(directionCoefficient * (int)(100.f * Game::GetGameClock()->GetTotalSeconds())));

	// Bad hack to make it work in the other direction
	// Will not work correctly if the game runs for over 2700 hours
	if (time < 0.f)
	{
		time += 9999999;
	}

	int target = (time % (2 * axisLength)) - frontRange;
	int displacement = coordAlongAxis - target;
	int distance = AbsoluteValue(displacement);

	Rgba finalColor = baseColor;

	float t = 0.f;
	if (displacement >= 0 && displacement <= frontRange)
	{
		t = (float)(frontRange - distance) / (float)frontRange;
		finalColor = Interpolate(baseColor, Rgba::WHITE, t);
	}
	else if (displacement < 0 && displacement > -rearRange)
	{
		t = (float)(rearRange - distance) / (float)(rearRange);
		finalColor = Interpolate(baseColor, Rgba::WHITE, t);
	}

	return finalColor;
}
