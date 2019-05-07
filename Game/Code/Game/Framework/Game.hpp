/************************************************************************/
/* File: Game.hpp
/* Author: Andrew Chase
/* Date: March 24th, 2018
/* Description: Class used for managing and updating game objects and 
/*              mechanics
/************************************************************************/
#pragma once
#include <vector>
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Transform.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Time/Stopwatch.hpp"
#include "Engine/Rendering/Core/Renderable.hpp"

class Clock;
class Camera;
class NamedProperties;

class Game
{
	
public:
	//-----Public Methods-----

	static void Initialize();
	static void ShutDown();
	
	void ProcessInput();				// Process all input this frame
	void Update();						// Updates all game object states, called each frame
	void Render() const;				// Renders all game objects to screen, called each frame

	static Game*				GetInstance();

	static Clock*				GetGameClock();
	static Camera*				GetGameCamera();
	static float				GetDeltaTime();


private:
	//-----Private Methods-----
	
	Game();
	~Game();
	Game(const Game& copy) = delete;

	void ProcessEventSystemInput();
	void ProcessJobSystemInput();
	void RunNamedPropertiesTest();

	static bool EventSystemStaticCallback(NamedProperties& args);
	bool		EventSystemObjectMethodCallback(NamedProperties& args);


public:
	//-----Public Data-----

	// For testing EventSystem
	bool m_cFunctionSubscribed = true;
	bool m_cFunctionShouldConsume = false;
	bool m_staticFunctionSubscribed = true;
	bool m_staticFunctionShouldConsume = false;
	bool m_objectMethodSubscribed = true;
	bool m_objectMethodShouldConsume = false;
	static constexpr float EVENT_FIRED_DURATION = 3.0f; // Seconds

	bool m_eventJustFired = false;

	Stopwatch m_eventFiredTimer;

	std::string m_eventResultsText;

	// Job System Testing
	int m_totalCreatedJobs = 0;
	int m_numJobsFinished = 0;

	std::vector<std::string> m_workerThreadIDs;


private:
	//-----Private Data-----

	Camera*	m_gameCamera = nullptr;
	Clock*	m_gameClock = nullptr;

	static Game* s_instance;			// The singleton Game instance

};

// C Function Event Callback
bool EventSystemCCallback(NamedProperties& args);