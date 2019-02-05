/************************************************************************/
/* File: Particle.cpp
/* Author: Andrew Chase
/* Date: October 8th, 2018
/* Description: Implementation of the Particle class
/************************************************************************/
#include "Game/Framework/Game.hpp"
#include "Game/Framework/World.hpp"
#include "Game/Entity/Particle.hpp"
#include "Game/Animation/VoxelAnimator.hpp"
#include "Game/Entity/Components/PhysicsComponent.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Utility/HeatMap.hpp"
#include "Engine/Core/Utility/ErrorWarningAssert.hpp"

//-----------------------------------------------------------------------------------------------
// Constructor - assembles the texture 
//
Particle::Particle(const Rgba& color, float lifetime, const Vector3& position, const Vector3& initialVelocity, bool attachToGround /*= false*/, bool fillsHoles /*= false*/)
	: Entity(EntityDefinition::GetDefinition("Particle"))
{
	ASSERT_OR_DIE(m_defaultSprite == nullptr, "Error: Particle definition has a default texture!");

	m_defaultSprite = new VoxelSprite();
	m_defaultSprite->CreateFromColorStream(&color, IntVector3(1, 1, 1), true);

	m_position = position;
	m_lifetime = lifetime;
	m_attachToGround = attachToGround;
	m_fillsHoles = fillsHoles;

	m_physicsComponent->SetVelocity(initialVelocity);
	m_stopwatch.SetClock(Game::GetGameClock());
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
Particle::~Particle()
{
}


//-----------------------------------------------------------------------------------------------
// Update loop
//
void Particle::Update()
{
	if (Game::GetWorld()->IsEntityOnGround(this) && GetCoordinatePosition().y > 0)
	{
		m_physicsEnabled = false;
	}
	else
	{
		m_physicsEnabled = true;
	}

	if (m_stopwatch.HasIntervalElapsed())
	{
		m_isMarkedForDelete = true;
	}
}


//-----------------------------------------------------------------------------------------------
// Starts the particle's lifetime timer
//
void Particle::OnSpawn()
{
	m_stopwatch.SetInterval(m_lifetime);
}


//-----------------------------------------------------------------------------------------------
// If flagged to attach, have the particle become part of the ground
//
void Particle::OnGroundCollision()
{
	if (m_attachToGround)
	{
		IntVector3 coordPosition = GetCoordinatePosition();
		World* world = Game::GetWorld();
		float yVelocity = m_physicsComponent->GetVelocity().y;

		// Ensure:
		// Particle is within XZ bounds
		bool isInMapBounds = world->IsEntityOnMap(this);
		
		// Particle is falling down
		bool isFallingDown = yVelocity < 0.f;

		// Particle didn't fall into a hole or can fill holes
		int height = world->GetMapHeightForEntity(this);
		bool isNotInHole = coordPosition.y > 0;

		if (isInMapBounds && isFallingDown && (isNotInHole || m_fillsHoles))
		{
			// Clamp to zero height to avoid going below the world
			coordPosition.y = ClampInt(coordPosition.y, 0, 256);

			world->AddVoxelToMap(coordPosition, m_defaultSprite->GetColorAtIndex(0));
			m_isMarkedForDelete = true;
		}
	}
}
