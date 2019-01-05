/************************************************************************/
/* File: Game.hpp
/* Author: Andrew Chase
/* Date: March 24th, 2018
/* Description: Class used for managing and updating game objects and 
/*              mechanics
/************************************************************************/
#pragma once
#include <vector>
#include "Game/Framework/Leaderboard.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Audio/AudioSystem.hpp"

class Clock;
class World;
class CampaignManager;
class GameCamera;
class Player;
class GameState;
class VoxelGrid;
class VoxelFont;

#define MAX_PLAYERS (4)
#define PLAYER_DEATH_PENALTY (-2000)
#define NUM_LEADERBOARDS (4)

enum eGameStateState
{
	GAME_STATE_TRANSITIONING_IN,
	GAME_STATE_UPDATING,
	GAME_STATE_TRANSITIONING_OUT
};


class Game
{
	
public:
	friend class GameState_Playing;
	friend class GameState_Loading;

	//-----Public Methods-----

	static void Initialize();
	static void ShutDown();
	
	void ProcessInput();				// Process all input this frame
	void Update();						// Updates all game object states, called each frame
	void Render() const;				// Renders all game objects to screen, called each frame

	GameState* GetGameState() const;

	static Game*	GetInstance();
	static void		TransitionToGameState(GameState* newState);

	static VoxelFont*			GetMenuFont();
	static VoxelGrid*			GetVoxelGrid();
	static Clock*				GetGameClock();
	static GameCamera*			GetGameCamera();
	static float				GetDeltaTime();
	static World*				GetWorld();
	static void 				SetWorld(World* world);
	static Player**				GetPlayers();
	static int					GetCurrentPlayerCount();
	static const Leaderboard*	GetLeaderboards();
	static CampaignManager*		GetCampaignManager();

	static bool					IsPlayerAlive(unsigned int index);

	static void					ResetScore();
	static void					AddPointsToScore(int pointsToAdd);
	static void					UpdateLeaderboardWithCurrentScore();
	static int					GetScore();

	static void					DrawPlayerHUD();
	static void					DrawScore();
	static void					DrawHeading(const std::string& headingText);

	static bool					AreAllPlayersInitialized();

	static void					PlayBGM(const std::string filename);

private:
	//-----Private Methods-----
	
	Game();
	~Game();
	Game(const Game& copy) = delete;

	void CheckForPlayers();

	void LoadLeaderboardsFromFile();
	void WriteLeaderboardsToFile();


private:
	//-----Private Data-----

	bool			m_doneLoading = false;
	GameState*		m_currentState = nullptr;
	GameState*		m_transitionState = nullptr;
	eGameStateState m_gameStateState = GAME_STATE_TRANSITIONING_IN;

	Clock*				m_gameClock = nullptr;
	World*				m_world = nullptr;
	CampaignManager*	m_campaignManager = nullptr;
	VoxelGrid*			m_voxelGrid = nullptr;
	Player*				m_players[MAX_PLAYERS];

	// Camera
	GameCamera*			m_gameCamera = nullptr;

	// In-game hud
	VoxelFont* m_hudFont = nullptr;
	VoxelFont* m_menuFont = nullptr;

	// Gameplay
	int	m_score = 0;
	Leaderboard m_leaderboards[NUM_LEADERBOARDS];

	// Audio
	SoundPlaybackID	m_bgm;

	static Game* s_instance;			// The singleton Game instance

};
