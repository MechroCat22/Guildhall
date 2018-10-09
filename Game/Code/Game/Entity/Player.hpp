/************************************************************************/
/* File: Player.hpp
/* Author: Andrew Chase
/* Date: September 22nd, 2017
/* Description: Class to represent a player entity
/************************************************************************/
#pragma once
#include "Game/Entity/MovingEntity.hpp"

#define INVALID_PLAYER_ID (4)

class Vector3;

class Player : public MovingEntity
{
public:
	//-----Public Methods-----

	// Initialization
	Player(unsigned int playerID);
	~Player();

	// Core Loop
	void			ProcessInput();
	virtual void	Update() override;

	// Events
	virtual void	OnCollision(Entity* other) override;
	virtual void	OnDamageTaken(int damageAmount) override;
	virtual void	OnDeath() override;
	virtual void	OnSpawn() override;

	// Behavior
	void Shoot();


private:
	//-----Private Methods-----
	
	//void ApplyInputAcceleration(const Vector2& inputDirection);
	//void ApplyDeceleration();


private:
	//-----Private Data-----

	int		m_playerID = INVALID_PLAYER_ID;

	//float	m_maxMoveAcceleration	= 300.f;
	//float	m_maxMoveSpeed			= 40.f;
	//float	m_maxMoveDeceleration	= 100.f;
	//float	m_jumpImpulse			= 80.f;

};