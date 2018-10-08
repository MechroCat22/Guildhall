/************************************************************************/
/* File: Entity.cpp
/* Author: Andrew Chase
/* Date: September 22nd, 2017
/* Description: Implementation of the Entity class
/************************************************************************/
#include "Game/Entity/Entity.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Animation/VoxelSprite.hpp"
#include "Game/Animation/VoxelAnimator.hpp"
#include "Game/Entity/PhysicsComponent.hpp"


#include "Engine/Core/Rgba.hpp"
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Rendering/DebugRendering/DebugRenderSystem.hpp"
#include <stdlib.h>


//-----------------------------------------------------------------------------------------------
// Constructor - takes the definition for its constant data characteristics
//
Entity::Entity(const EntityDefinition* definition)
	: m_definition(definition)
{
	if (m_definition->m_physicsType == PHYSICS_TYPE_DYNAMIC)
	{
		m_physicsComponent = new PhysicsComponent(this);
	}

	m_animator = new VoxelAnimator(m_definition->m_animationSet, m_definition->m_defaultSprite);
	m_animator->Play("idle");

	m_position = Vector3(GetRandomFloatInRange(20.f, 100.f), 4.f, GetRandomFloatInRange(20.f, 100.f));
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
Entity::~Entity()
{
}


//-----------------------------------------------------------------------------------------------
// Update
//
void Entity::Update()
{
}


//-----------------------------------------------------------------------------------------------
// Callback for when collision is detected
//
void Entity::OnCollision(Entity* other)
{
	UNUSED(other);
}


//-----------------------------------------------------------------------------------------------
// Adds an offset (translates) to the player
//
void Entity::AddPositionOffset(const Vector3& offset)
{
	m_position += offset;
}


//-----------------------------------------------------------------------------------------------
// Hard sets the position of the entity
//
void Entity::SetPosition(const Vector3& newPosition)
{
	m_position = newPosition;
}


//-----------------------------------------------------------------------------------------------
// Sets the orientation of the entity to the provided value
//
void Entity::SetOrientation(float orientation)
{
	m_orientation = orientation;
}


//-----------------------------------------------------------------------------------------------
// Subtracts the amount to the entity's health
//
void Entity::TakeDamage(int damageAmount)
{
	m_health -= damageAmount;

	OnDamageTaken(damageAmount);

	if (m_health <= 0)
	{
		OnDeath();
	}
}


//-----------------------------------------------------------------------------------------------
// Called when the entity takes damage, for custom behavior
//
void Entity::OnDamageTaken(int damageAmount)
{
	UNUSED(damageAmount);
}


//-----------------------------------------------------------------------------------------------
// Called when the entity's health is <= 0
//
void Entity::OnDeath()
{
}


//-----------------------------------------------------------------------------------------------
// Called when the entity is spawned
//
void Entity::OnSpawn()
{
}


//-----------------------------------------------------------------------------------------------
// Adds the correction to the entity's position to fix a collision
//
void Entity::AddCollisionCorrection(const Vector3& correction)
{
	m_position += correction;
}


//-----------------------------------------------------------------------------------------------
// Returns the world position of the entity
//
Vector3 Entity::GetEntityPosition() const
{
	return m_position;
}


//-----------------------------------------------------------------------------------------------
// Returns the 3D texture to used for rendering, based on the current 2D orientation of the entity
//
const VoxelTexture* Entity::GetTextureForOrientation() const
{
	const VoxelSprite* sprite = m_animator->GetCurrentSprite();
	return sprite->GetTextureForOrientation(m_orientation);
}


//-----------------------------------------------------------------------------------------------
// Returns the collision definition of this entity
//
CollisionDefinition_t Entity::GetCollisionDefinition() const
{
	return m_definition->m_collisionDef;
}


//-----------------------------------------------------------------------------------------------
// Returns the physics type of the entity
//
ePhysicsType Entity::GetPhysicsType() const
{
	return m_definition->m_physicsType;
}


//-----------------------------------------------------------------------------------------------
// Returns the definition used to describe this entity's data characteristics
//
const EntityDefinition* Entity::GetEntityDefinition() const
{
	return m_definition;
}


//-----------------------------------------------------------------------------------------------
// Returns the physics component of this entity
//
PhysicsComponent* Entity::GetPhysicsComponent() const
{
	return m_physicsComponent;
}


//-----------------------------------------------------------------------------------------------
// Returns the mass of the entity
//
float Entity::GetMass() const
{
	return m_mass;
}


//-----------------------------------------------------------------------------------------------
// Returns one over the mass of the entity
//
float Entity::GetInverseMass() const
{
	return m_inverseMass;
}


//-----------------------------------------------------------------------------------------------
// Returns whether this entity has been marked to be deleted at the end of the frame
//
bool Entity::IsMarkedForDelete() const
{
	return m_isMarkedForDelete;
}


//-----------------------------------------------------------------------------------------------
// Returns the position of the entity's voxel at the given local coords
//
Vector3 Entity::GetPositionForLocalCoords(const IntVector3& localCoords) const
{
	IntVector3 halfDimensions = m_dimensions / 2;

	IntVector3 entityPositionCoords = GetEntityCoordinatePosition();

	IntVector3 bottomLeft = entityPositionCoords - IntVector3(halfDimensions.x, 0, halfDimensions.z);

	Vector3 position = Vector3(bottomLeft + localCoords);
	position += Vector3(0.5f, 0.f, 0.5f);

	return position;
}


//-----------------------------------------------------------------------------------------------
// Returns the position of the entity's voxel at the given index
//
Vector3 Entity::GetPositionForLocalIndex(unsigned int index) const
{
	int y = index / (m_dimensions.x * m_dimensions.z);
	int leftOver = index % (m_dimensions.x * m_dimensions.z);

	int z = leftOver / (m_dimensions.x);
	int x = leftOver % (m_dimensions.x);

	return GetPositionForLocalCoords(IntVector3(x, y, z));
}


//-----------------------------------------------------------------------------------------------
// Returns the coordinate position that this entity occupies
//
IntVector3 Entity::GetEntityCoordinatePosition() const
{
	return IntVector3(m_position);
}
