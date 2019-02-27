/************************************************************************/
/* File: GameCamera.cpp
/* Author: Andrew Chase
/* Date: September 28th 2018
/* Description: Implementation of the GameCamera class
/************************************************************************/
#include "Game/Entity/Player.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/Framework/GameCamera.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Input/InputSystem.hpp"

//-----------------------------------------------------------------------------------------------
// Constructor
//
GameCamera::GameCamera()
	: m_offsetDirection(Vector3(0.f, 1.f, -1.3f).GetNormalized())
	, m_offsetDistance(150.f)
{
	m_screenShakeInterval.Reset();
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
GameCamera::~GameCamera()
{
}


//-----------------------------------------------------------------------------------------------
// Checks for input that directly moves the camera and caches it off for update
//
void GameCamera::ProcessInput()
{
	float deltaTime = Game::GetDeltaTime();
	InputSystem* input = InputSystem::GetInstance();

	// Translating the camera
	Vector3 translationOffset = Vector3::ZERO;
	if (input->IsKeyPressed('W')) { translationOffset.z += 1.f; }									// Forward
	if (input->IsKeyPressed('S')) { translationOffset.z -= 1.f; }									// Left
	if (input->IsKeyPressed('A')) { translationOffset.x -= 1.f; }									// Back
	if (input->IsKeyPressed('D')) { translationOffset.x += 1.f; }									// Right
	if (input->IsKeyPressed(InputSystem::KEYBOARD_SPACEBAR)) { translationOffset.y += 1.f; }		// Up
	if (input->IsKeyPressed('X')) { translationOffset.y -= 1.f; }									// Down

	if (input->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
	{
		translationOffset *= 50.f;
	}

	translationOffset *= CAMERA_TRANSLATION_SPEED * deltaTime;

	m_frameTranslation = translationOffset;

	// Rotating the camera
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();

	Vector2 rotationOffset = Vector2((float)mouseDelta.y, (float)mouseDelta.x) * 0.12f;
	m_frameRotation = Vector3(rotationOffset.x * CAMERA_ROTATION_SPEED * deltaTime, rotationOffset.y * CAMERA_ROTATION_SPEED * deltaTime, 0.f);

	// Check for updating state
	if (input->WasKeyJustPressed('B'))
	{
		m_state = (eCameraState) (m_state + 1);
		if (m_state >= NUM_STATES)
		{
			m_state = (eCameraState)0;
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Updates the camera to move based on its current state
//
void GameCamera::UpdateBasedOnState()
{
	switch (m_state)
	{
	case STATE_FIXED:
		UpdateFixed();
		break;
	case STATE_FOLLOW:
		UpdateFollow();
		break;
	case STATE_FREE:
		UpdateFree();
		break;
	default:
		break;
	}
}


//-----------------------------------------------------------------------------------------------
// Updates the camera to be positioned relative to the players' locations
//
void GameCamera::UpdateFollow()
{
	Player** players = Game::GetPlayers();

	// Find the middle x and z positions
	IntVector3 targetPos = IntVector3::ZERO;
	IntVector3 playerDimensions;
	int minX = 3000;
	int minY = 3000;
	int minZ = 3000;
	int maxX = -1;
	int maxY = -1;
	int maxZ = -1;

	bool foundPlayer = false;
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (players[i] != nullptr && !players[i]->IsRespawning())
		{
			foundPlayer = true;
			playerDimensions = players[i]->GetOrientedDimensions();

			IntVector3 pos = players[i]->GetCoordinatePosition();

			minX = MinInt(minX, pos.x);
			maxX = MaxInt(maxX, pos.x);

			minY = MinInt(minY, pos.y);
			maxY = MaxInt(maxY, pos.y);

			minZ = MinInt(minZ, pos.z);
			maxZ = MaxInt(maxZ, pos.z);
		}
	}

	if (!foundPlayer)
	{
		return;
	}

	Vector3 finalTarget = 0.5f * (Vector3(minX, minY, minZ) + Vector3(maxX, maxY, maxZ)) + Vector3(playerDimensions / 2);
	finalTarget.y = 5.f;

	Vector3 newPos = finalTarget + m_offsetDirection * m_offsetDistance;
	LookAt(newPos, finalTarget);
}


//-----------------------------------------------------------------------------------------------
// Updates the camera by checking for input
//
void GameCamera::UpdateFree()
{
	TranslateLocal(m_frameTranslation);
	Rotate(m_frameRotation);

	m_frameTranslation = Vector3::ZERO;
	m_frameRotation = Vector3::ZERO;
}


//-----------------------------------------------------------------------------------------------
// Sets the camera back to it's offset position looking at the grid's center
//
void GameCamera::UpdateFixed()
{
	
	Vector3 target = Vector3(128.f, 0.f, 120.f);
	Vector3 newPos = Vector3(128.f, 185.f, -90.f);
	LookAt(newPos, target);

	// Apply screenshake in fixed only
	float magnitude = m_screenShakeInterval.GetTimeUntilIntervalEnds();
	Vector3 localDirection = magnitude * Vector3(GetRandomFloatZeroToOne(), GetRandomFloatZeroToOne(), 0.f);

	TranslateLocal(localDirection);
}


//-----------------------------------------------------------------------------------------------
// Adds the given screenshake magnitude to the current running amount
//
void GameCamera::AddScreenShake(float addedScreenShakeMagnitude)
{
	float currentIntervalRemaining = m_screenShakeInterval.GetTimeUntilIntervalEnds();
	float newInterval = currentIntervalRemaining + addedScreenShakeMagnitude;
	m_screenShakeInterval.SetInterval(newInterval);
}
