/************************************************************************/
/* File: Player.cpp
/* Author: Andrew Chase
/* Date: September 22nd, 2017
/* Description: Implementation of the player class
/************************************************************************/
#include "Game/Entity/Player.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/Framework/World.hpp"
#include "Game/Entity/Projectile.hpp"
#include "Engine/Core/Window.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"
#include "Engine/Rendering/DebugRendering/DebugRenderSystem.hpp"

//-----------------------------------------------------------------------------------------------
// Constructor
//
Player::Player(unsigned int playerID)
	: DynamicEntity()
	, m_playerID(playerID)
{
	m_collisionDef = CollisionDefinition_t(COLLISION_TYPE_BOX, 8.f, 8.f, 8.f);

	SetupVoxelTextures("Data/3DTextures/TestCube.qef");
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
Player::~Player()
{
}


//-----------------------------------------------------------------------------------------------
// Checks for player related input on the respective controller
//
void Player::ProcessInput()
{
	UpdateMovementParamsOnInput();
	DebugRenderMovementParams();

	XboxController& controller = InputSystem::GetInstance()->GetController(m_playerID);
	Vector2 leftStick = controller.GetCorrectedStickPosition(XBOX_STICK_LEFT);

	float currSpeed = m_velocity.GetLength();
	float deltaTime = Game::GetDeltaTime();

	Vector3 inputDirection = Vector3(leftStick.x, 0.f, leftStick.y);

	// If we have input, apply a movement force
	if (inputDirection != Vector3::ZERO)
	{
		ApplyInputAcceleration(inputDirection);
	}

	// If we have no input or are moving too fast, decelerate
	if (leftStick == Vector2::ZERO || (currSpeed > m_maxMoveSpeed))
	{
		ApplyDeceleration();
	}

	// Test adding a force
	if (controller.WasButtonJustPressed(XBOX_BUTTON_X))
	{
		AddForce(inputDirection * -10000.f);
	}

	// Test shooting
	if (controller.IsButtonPressed(XBOX_BUTTON_A))
	{
		Shoot();
	}
}


//-----------------------------------------------------------------------------------------------
// Update
//
void Player::Update()
{
	DynamicEntity::Update();
}


//-----------------------------------------------------------------------------------------------
// Collision callback
//
void Player::OnCollision(Entity* other)
{
	DynamicEntity::OnCollision(other);
}


//-----------------------------------------------------------------------------------------------
// On damage callback
//
void Player::OnDamageTaken(int damageAmount)
{
	DynamicEntity::OnDamageTaken(damageAmount);
}


//-----------------------------------------------------------------------------------------------
// On death callback
//
void Player::OnDeath()
{
	DynamicEntity::OnDeath();
}


//-----------------------------------------------------------------------------------------------
// On spawn callback
//
void Player::OnSpawn()
{
	DynamicEntity::OnSpawn();
}


//-----------------------------------------------------------------------------------------------
// Shoots a projectile
//
void Player::Shoot()
{
	Projectile* proj = new Projectile();
	proj->SetPosition(m_position);
	proj->SetOrientation(m_orientation);

	Vector2 direction = Vector2::MakeDirectionAtDegrees(m_orientation);	
	proj->SetVelocity(Vector3(direction.x, 0.f, direction.y) * 100.f);
	
	World* world = Game::GetWorld();
	world->AddDynamicEntity(proj);
}


//-----------------------------------------------------------------------------------------------
// Checks for input to update the player's base movement parameters
//
void Player::UpdateMovementParamsOnInput()
{
	InputSystem* input = InputSystem::GetInstance();
	float deltaTime = Game::GetDeltaTime();

	float maxAccelChange = 0.f;
	if (input->IsKeyPressed('T'))
	{
		maxAccelChange -= 20.f * deltaTime;
	}

	if (input->IsKeyPressed('Y'))
	{
		maxAccelChange += 20.f * deltaTime;
	}

	m_maxMoveAcceleration += maxAccelChange;

	float maxDecelChange = 0.f;
	if (input->IsKeyPressed('O'))
	{
		maxDecelChange -= 20.f * deltaTime;
	}

	if (input->IsKeyPressed('P'))
	{
		maxDecelChange += 20.f * deltaTime;
	}

	m_maxMoveDeceleration += maxDecelChange;

	float maxSpeedChange = 0.f;
	if (input->IsKeyPressed('U'))
	{
		maxSpeedChange -= 20.f * deltaTime;
	}

	if (input->IsKeyPressed('I'))
	{
		maxSpeedChange += 20.f * deltaTime;
	}

	m_maxMoveSpeed += maxSpeedChange;
}


//-----------------------------------------------------------------------------------------------
// Renders the player's current movement params and hotkeys for changing
//
void Player::DebugRenderMovementParams()
{
	AABB2 bounds = Window::GetInstance()->GetWindowBounds();
	std::string toPrint = Stringf("(T,Y) Max Acceleration: %.2f", m_maxMoveAcceleration);
	toPrint += Stringf("\n(U,I) Max Deceleration: %.2f", m_maxMoveDeceleration);
	toPrint += Stringf("\n(O,P) Max Speed: %.2f", m_maxMoveSpeed);

	DebugRenderSystem::Draw2DText(toPrint, bounds, 0.f, Rgba::WHITE, 30.f);
}

void Player::ApplyInputAcceleration(const Vector3& inputDirection)
{
	float currSpeed = m_velocity.GetLength();
	float deltaTime = Game::GetDeltaTime();

	Vector3 finalVelocity = (m_velocity + (m_maxMoveAcceleration * deltaTime) * inputDirection);
	float finalSpeed = finalVelocity.NormalizeAndGetLength();

	finalSpeed = (currSpeed > m_maxMoveSpeed ? ClampFloat(finalSpeed, 0.f, currSpeed) : ClampFloat(finalSpeed, 0.f, m_maxMoveSpeed));
	finalVelocity *= finalSpeed;

	Vector3 inputVelocityResult = finalVelocity - m_velocity;
	Vector3 acceleration = inputVelocityResult / deltaTime;
	Vector3 force = acceleration * m_mass;

	AddForce(force);

	// Reorient the player
	m_orientation = inputDirection.xz().GetOrientationDegrees();
}


void Player::ApplyDeceleration()
{
	float deltaTime = Game::GetDeltaTime();
	float currSpeed = m_velocity.GetLength();
	float amountCanBeDecreased = currSpeed;

	if (amountCanBeDecreased > 0.f)
	{
		Vector3 direction = -1.0f * m_velocity.GetNormalized();

		float decelMag = amountCanBeDecreased / deltaTime;
		decelMag = ClampFloat(decelMag, 0.f, m_maxMoveDeceleration);

		float forceMag = decelMag * m_mass;

		Vector3 force = forceMag * direction;
		AddForce(force);
	}
}
