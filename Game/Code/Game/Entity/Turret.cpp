/************************************************************************/
/* File: Turret.cpp
/* Author: Andrew Chase
/* Date: June 18th, 2018
/* Description: Implementation of the Turret class
/************************************************************************/
#include "Game/Entity/Turret.hpp"
#include "Game/Entity/Cannon.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/Framework/GameCommon.hpp"

#include "Engine/Math/Matrix44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Rendering/Core/Renderable.hpp"
#include "Engine/Rendering/Core/RenderScene.hpp"

// Constants
const float Turret::TURRET_ROTATION_SPEED = 60.f;


//-----------------------------------------------------------------------------------------------
// Constructor
//
Turret::Turret(Transform& parent)
{
	transform.SetParentTransform(&parent);
	transform.position = Vector3(0.f, 1.3f, 0.f);

	// Set up the tank base renderable
	m_renderable = new Renderable();
	RenderableDraw_t draw;
	draw.sharedMaterial = AssetDB::GetSharedMaterial("Data/Materials/Tank.material");
	draw.mesh = AssetDB::GetMesh("Cube");
	draw.drawMatrix = Matrix44::MakeModelMatrix(Vector3::ZERO, Vector3::ZERO, Vector3(1.3f, 0.9f, 1.3f));

	m_renderable->AddDraw(draw);
	m_renderable->AddInstanceMatrix(transform.GetWorldMatrix());

	Game::GetRenderScene()->AddRenderable(m_renderable);

	// Make the cannon child to our transform
	m_cannon = new Cannon(transform);
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
Turret::~Turret()
{
	delete m_cannon;
	m_cannon = nullptr;

	Game::GetRenderScene()->RemoveRenderable(m_renderable);
	delete m_renderable;
	m_renderable = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Update
//
void Turret::Update(float deltaTime)
{
	m_renderable->SetInstanceMatrix(0, transform.GetWorldMatrix());
	m_cannon->Update(deltaTime);
}


//-----------------------------------------------------------------------------------------------
// Returns the cannon of this turret
//
Cannon* Turret::GetCannon() const
{
	return m_cannon;
}


//-----------------------------------------------------------------------------------------------
// Turns the turret and it's cannon towards the target given
// Turret rotation is restricted to its local XZ-plane
//
void Turret::TurnTowardsTarget(const Vector3& target)
{
	// Get Transforms
	Matrix44 toParent = transform.GetLocalMatrix();
	Matrix44 toLocal = transform.GetWorldMatrix().GetInverse();

	// Transform the target to local space, and snap it to the xz-plane
	Vector3 localPosition = toLocal.TransformPoint(target).xyz();
	localPosition.y = 0.f;

	// Transform it back to the parent's space
	Vector3 parentPosition = toParent.TransformPoint(localPosition).xyz();

	// Get the current and desired angles
	Vector3 currentRotation = transform.rotation.GetAsEulerAngles();
	Vector2 dirToTarget = (parentPosition - transform.position).xz();

	float startAngle = currentRotation.y;
	float endAngle = 180.f - (dirToTarget.GetOrientationDegrees() + 90.f);

	// Find the resulting angle
	float deltaTime = Game::GetDeltaTime();
	float newAngle = TurnToward(startAngle, endAngle, TURRET_ROTATION_SPEED * deltaTime);

	// Set the rotation
	currentRotation.y = newAngle;
	transform.rotation = Quaternion::FromEuler(currentRotation);

	// Rotate the cannon
	m_cannon->ElevateTowardsTarget(target);
}
