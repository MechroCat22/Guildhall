/************************************************************************/
/* File: GameState.hpp
/* Author: Andrew Chase
/* Date: February 12th, 2018
/* Description: Abstract class to represent a given Game's state
/************************************************************************/
#pragma once
#include "Game/Game.hpp"


class GameState
{
public:
	//-----Public Methods-----

	virtual void ProcessInput() = 0;
	virtual void Update() = 0;
	virtual void Render() const = 0;

	virtual void Enter() = 0;
	virtual void Leave() = 0;

private:
	//-----Private Data-----

};
