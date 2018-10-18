/************************************************************************/
/* File: Game.cpp
/* Author: Andrew Chase
/* Date: March 24th, 2018
/* Description: Game class for general gameplay management
/************************************************************************/
#include "Game/Framework/Game.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/GameStates/GameState_Loading.hpp"

#include "Engine/Core/Time/Clock.hpp"
#include "Engine/Networking/NetSession.hpp"
#include "Engine/Rendering/Core/Camera.hpp"
#include "Engine/Networking/NetMessage.hpp"
#include "Engine/Rendering/Core/Renderer.hpp"
#include "Engine/Rendering/Core/RenderScene.hpp"
#include "Engine/Networking/NetConnection.hpp"

#include "Engine/Networking/Socket.hpp"
#include "Engine/Core/DeveloperConsole/Command.hpp"

// Port the net session will run on
#define GAME_PORT 10084

// Message IDs for game messages
enum eNetGameMessage : uint8_t
{
	NET_MSG_GAME_TEST = NET_MSG_CORE_COUNT,

	// Other messages

	NET_MSG_UNRELIABLE_TEST = 128
};

// Message callbacks
bool UnreliableTest(NetMessage* msg, const NetSender_t& sender);


// Commands for testing the NetSession

//-----------------------------------------------------------------------------------------------
// Sends an add request message to the given connection index
//
void Command_AddConnection(Command& cmd)
{
	int index = -1;
	if (!cmd.GetParam("i", index))
	{
		ConsoleErrorf("No index specified");
		return;
	}

	if (index < 0)
	{
		ConsoleErrorf("Invalid index");
		return;
	}

	std::string addr;
	if (!cmd.GetParam("a", addr))
	{
		ConsoleErrorf("No address specified");
		return;
	}

	NetAddress_t netAddr(addr.c_str());
	NetSession* session = Game::GetNetSession();

	if (session == nullptr)
	{
		ConsoleErrorf("No Session set up in game");
		return;
	}

	bool added = session->AddConnection((uint8_t)index, netAddr);

	if (added)
	{
		ConsolePrintf(Rgba::GREEN, "Connection to %s added at index %i", addr.c_str(), (uint8_t)index);
	}
	else
	{
		ConsoleErrorf("Couldn't add connection to %s at index %i", addr.c_str(), (uint8_t)index);
	}
}


//-----------------------------------------------------------------------------------------------
// Sends a ping message to the given connection index
//
void Command_SendPing(Command& cmd)
{
	int connectionIndex = -1;
	if (!cmd.GetParam("i", connectionIndex))
	{
		ConsoleErrorf("No connection index specified");
		return;
	}

	NetConnection* connection = Game::GetNetSession()->GetConnection((uint8_t)connectionIndex);

	if (connection == nullptr)
	{
		ConsoleErrorf("Could not find connection at index %i", connectionIndex);
		return;
	}

	uint8_t definitionIndex;
	const NetMessageDefinition_t* definition = Game::GetNetSession()->GetMessageDefinition("ping");

	if (definition == nullptr)
	{
		ConsoleErrorf("Definition does not exist on NetSession for message \"ping\"");
		return;
	}

	NetMessage* msg = new NetMessage(definition);
	msg->WriteString("Hello, World!");

	connection->Send(msg);
}


//-----------------------------------------------------------------------------------------------
// Command for setting the NetSession simulated latency
//
void Command_SetNetSimLag(Command& cmd)
{
	float min = 0.1f;
	cmd.GetParam("min", min, &min);

	float max = min;
	cmd.GetParam("max", max, &max);

	NetSession* session = Game::GetNetSession();
	session->SetSimLatency(min, max);
}


//-----------------------------------------------------------------------------------------------
// Command for setting the NetSession simulated packet loss
//
void Command_SetNetSimLoss(Command& cmd)
{
	float loss = 0.f;
	cmd.GetParam("a", loss, &loss);

	NetSession* session = Game::GetNetSession();
	session->SetSimLoss(loss);
}


//-----------------------------------------------------------------------------------------------
// Command for sending the NetSession tick rate
//
void Command_SetSessionNetTick(Command& cmd)
{
	float hertz = 60.f;
	bool provided = cmd.GetParam("f", hertz, &hertz);

	float timeInterval;

	if (provided && hertz > 0.f)
	{
		timeInterval = (1.f / hertz);
	}
	else
	{
		timeInterval = 0;
	}

	ConsolePrintf(Rgba::GREEN, "Setting the NetSession tick rate to %f hertz", timeInterval);

	Game::GetNetSession()->SetNetTickRate(hertz);
}


//-----------------------------------------------------------------------------------------------
// Command for setting a NetConnection tick rate
//
void Command_SetConnectionNetTick(Command& cmd)
{
	int index = -1;
	cmd.GetParam("i", index);

	if (index <= -1)
	{
		ConsoleErrorf("No index (-i) specified");
		return;
	}

	float hertz = 0.f;
	bool provided = cmd.GetParam("h", hertz);

	float timeInterval;

	if (provided && hertz > 0.f)
	{
		timeInterval = (1.f / hertz);
	}
	else
	{
		timeInterval = 0.f;
	}

	ConsolePrintf(Rgba::GREEN, "Setting the NetConnection at index %i tick rate to %f between each send", index, timeInterval);

	Game::GetNetSession()->GetConnection((uint8_t)index)->SetNetTickRate(hertz);
}


//-----------------------------------------------------------------------------------------------
// Command for setting the heartbeat interval
//
void Command_SetHeartbeat(Command& cmd)
{
	float hertz = 2.f;
	cmd.GetParam("a", hertz);

	Game::GetNetSession()->SetConnectionHeartbeatInterval(hertz);

	ConsolePrintf(Rgba::GREEN, "Set the NetSession's heartbeat to %f hz", hertz);
}


//-----------------------------------------------------------------------------------------------
//--------------------------------- Game Class --------------------------------------------------
//-----------------------------------------------------------------------------------------------


// The singleton instance
Game* Game::s_instance = nullptr;

//-----------------------------------------------------------------------------------------------
// Default constructor, initialize any game members here (private)
//
Game::Game()
	: m_currentState(new GameState_Loading())
{
	// Clock
	m_gameClock = new Clock(Clock::GetMasterClock());

	// Camera
	Renderer* renderer = Renderer::GetInstance();
	m_gameCamera = new Camera();
	m_gameCamera->SetColorTarget(renderer->GetDefaultColorTarget());
	m_gameCamera->SetDepthTarget(renderer->GetDefaultDepthTarget());
	m_gameCamera->SetProjectionPerspective(45.f, 0.1f, 10000.f);
	m_gameCamera->LookAt(Vector3(0.f, 200.f, -500.0f), Vector3(0.f, 200.f, 0.f));

	// Render Scene
	m_renderScene = new RenderScene("Game Scene");
	m_renderScene->AddCamera(m_gameCamera);

	// Net Session
	m_netSession = new NetSession();
	RegisterGameMessages();
	m_netSession->Bind(GAME_PORT, 10);
}


//-----------------------------------------------------------------------------------------------
// Destructor - clean up any allocated members (private)
//
Game::~Game()
{
	delete m_netSession;
	m_netSession = nullptr;

	delete m_renderScene;
	m_renderScene = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Constructs the singleton game instance
//
void Game::Initialize()
{
	GUARANTEE_OR_DIE(s_instance == nullptr, "Error: Game::Initialize called when a Game instance already exists.");
	s_instance = new Game();

	// Set the game clock on the Renderer
	Renderer::GetInstance()->SetRendererGameClock(s_instance->m_gameClock);

	Command::Register("add_connection", "Adds a connection to the game session for the given index and address", Command_AddConnection);
	Command::Register("send_ping", "Sends a ping on the current net session to the given connection index", Command_SendPing);

	Command::Register("net_sim_lag", "Sets the simulated latency of the game net session", Command_SetNetSimLag);
	Command::Register("net_sim_loss", "Sets the simulated packet loss of the game net session", Command_SetNetSimLoss);

	Command::Register("net_set_session_send_rate", "Sets the NetSession's network tick rate", Command_SetSessionNetTick);
	Command::Register("net_set_connection_send_rate", "Sets the connection's tick rate at the specified index", Command_SetConnectionNetTick);

	Command::Register("net_set_heartbeat", "Sets the NetSession's heartbeat", Command_SetHeartbeat);
}


//-----------------------------------------------------------------------------------------------
// Deletes the singleton instance
//
void Game::ShutDown()
{
	delete s_instance;
	s_instance = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Checks for input received this frame and updates states accordingly
//
void Game::ProcessInput()
{
	m_currentState->ProcessInput();
}


//-----------------------------------------------------------------------------------------------
// Update the movement variables of every entity in the world, as well as update the game state
// based on the input this frame
//
void Game::Update()
{
	m_netSession->ProcessIncoming();

	// Check for state change
	CheckToUpdateGameState();

	// Update the current state
	m_currentState->Update();

	m_netSession->ProcessOutgoing();
}


//-----------------------------------------------------------------------------------------------
// Draws to screen
//
void Game::Render() const
{
	m_currentState->Render();
	m_netSession->RenderDebugInfo();
}


//-----------------------------------------------------------------------------------------------
// Sets the pending state flag to the one given, so the next frame the game will switch to the
// given state
//
void Game::TransitionToGameState(GameState* newState)
{
	s_instance->m_pendingState = newState;
}


//-----------------------------------------------------------------------------------------------
// Returns the game clock
//
Clock* Game::GetGameClock()
{
	return s_instance->m_gameClock;
}


//-----------------------------------------------------------------------------------------------
// Returns the camera used to render game elements
//
Camera* Game::GetGameCamera()
{
	return s_instance->m_gameCamera;
}


//-----------------------------------------------------------------------------------------------
// Returns the frame time for the game clock
//
float Game::GetDeltaTime()
{
	return s_instance->m_gameClock->GetDeltaTime();
}


//-----------------------------------------------------------------------------------------------
// Returns the game's render scene
//
RenderScene* Game::GetRenderScene()
{
	return s_instance->m_renderScene;
}


//-----------------------------------------------------------------------------------------------
// Returns the net session of the instance
//
NetSession* Game::GetNetSession()
{
	return s_instance->m_netSession;
}


//-----------------------------------------------------------------------------------------------
// Returns the singleton Game instance
//
Game* Game::GetInstance()
{
	return s_instance;
}


//-----------------------------------------------------------------------------------------------
// Checks if there is a pending 
//
void Game::CheckToUpdateGameState()
{
	// Have a state pending
	if (m_pendingState != nullptr)
	{
		if (m_currentState != nullptr)
		{
			// Leave and destroy current
			m_currentState->Leave();
			delete m_currentState;
		}

		// Set new as current
		m_currentState = m_pendingState;
		m_pendingState = nullptr;

		// Enter the new state
		m_currentState->Enter();
	}
}


//-----------------------------------------------------------------------------------------------
// Registers all game net messages to the game NetSession
//
void Game::RegisterGameMessages()
{
	m_netSession->RegisterMessageDefinition(NET_MSG_UNRELIABLE_TEST, "unreliable_test", UnreliableTest);
}


//-----------------------------------------------------------------------------------------------
// Message callbacks
//-----------------------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------------------
// For testing unreliable messages that require a connection
//
bool UnreliableTest(NetMessage* msg, const NetSender_t& sender)
{
	UNUSED(msg);
	UNUSED(sender);

	return true;
}
