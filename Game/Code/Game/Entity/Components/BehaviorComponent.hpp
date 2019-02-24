/************************************************************************/
/* File: BehaviorComponent.hpp
/* Author: Andrew Chase
/* Date: October 9th, 2018
/* Description: Class to hold any AI state and data of an entity
/************************************************************************/
#pragma once

class Player;
class Entity;
class AIEntity;

class BehaviorComponent
{
	friend class EntityDefinition;

public:
	//-----Public Methods-----
	
	BehaviorComponent();
	virtual ~BehaviorComponent();

	virtual void				Initialize(AIEntity* owningEntity);
	virtual void				Update();
	virtual BehaviorComponent*	Clone() const = 0;

	virtual void				OnSpawn();
	virtual void				OnEntityCollision(Entity* other);

	
protected:
	//-----Protected Methods-----

	Player*	GetClosestAlivePlayer() const;
	Player* GetClosestPlayerInSight() const;
	float	GetDistanceToClosestPlayer() const;

	void	MoveToClosestPlayer();

	Vector2 GetDirectionToAvoidClosestStaticObstacle(const Vector2& targetDirection) const;
	

protected:
	//-----Protected Data-----
	
	AIEntity*	m_owningEntity = nullptr;
	Player*		m_closestPlayer = nullptr;

	int			m_damageDealtOnTouch = 1;
	float		m_knockBackOnTouch = 100.f;

};
