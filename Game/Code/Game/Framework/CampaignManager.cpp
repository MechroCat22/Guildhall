/************************************************************************/
/* File: CampaignManager.cpp
/* Author: Andrew Chase
/* Date: October 20th 2018
/* Description: Implementation of the CampaignManager class
/************************************************************************/
#include "Game/Framework/Game.hpp"
#include "Game/Framework/World.hpp"
#include "Game/Framework/SpawnPoint.hpp"
#include "Game/Entity/EntityDefinition.hpp"
#include "Game/Framework/CampaignStageData.hpp"
#include "Game/Framework/CampaignDefinition.hpp"
#include "Game/Framework/MapDefinition.hpp"
#include "Game/Framework/CampaignManager.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"
#include "Engine/Core/Utility/XmlUtilities.hpp"
#include "Engine/Core/Utility/ErrorWarningAssert.hpp"

//-----------------------------------------------------------------------------------------------
// Constructor
//
CampaignManager::CampaignManager()
	: m_spawnClock(Game::GetGameClock())
	, m_stageTimer(Stopwatch(&m_spawnClock))
	, m_spawnTick(Stopwatch(&m_spawnClock))
{
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
CampaignManager::~CampaignManager()
{
	CleanUp();
}


//-----------------------------------------------------------------------------------------------
// Sets up the campaign manager to start a campaign with the given definition
//
void CampaignManager::Initialize(const CampaignDefinition* definition)
{
	// In case we have leftover state
	CleanUp();

	// Set the definition
	m_campaignDefinition = definition;

	// We don't need to clone the spawn events for the first stage, as it should be the
	// character select, which has none
}


//-----------------------------------------------------------------------------------------------
// Resets the manager to be used again in another game
//
void CampaignManager::CleanUp()
{
	// Basic state
	m_spawnTick.Reset();
	m_currStageFinished = false;
	m_currStageIndex = 0;
	m_totalSpawnedThisStage = 0;

	for (int i = 0; i < (int)m_currentSpawnEvents.size(); ++i)
	{
		delete m_currentSpawnEvents[i];
	}
	
	m_currentSpawnEvents.clear();

	// Data
	m_campaignDefinition = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Update
//
void CampaignManager::Update()
{
	// Check for spawn tick
	if (m_spawnTick.DecrementByIntervalAll() == 0)
	{
		return;
	}

	// Have all events run a spawn tick if they aren't finished
	int numEvents = (int) m_currentSpawnEvents.size();
	bool allEventsFinished = true;

	for (int eventIndex = 0; eventIndex < numEvents; ++eventIndex)
	{
		EntitySpawnEvent* currEvent = m_currentSpawnEvents[eventIndex];
		
		if (!currEvent->IsFinished())
		{
			allEventsFinished = false;
			
			int enemiesSpawned = currEvent->RunSpawn();
			m_totalSpawnedThisStage += enemiesSpawned;
		}
	}

	// End of stage check - all enemies dead
	if (allEventsFinished && (GetCurrentLiveEnemyCount() == 0))
	{
		m_currStageFinished = true;
	}
}


//-----------------------------------------------------------------------------------------------
// Sets up the manager to begin the next stage
//
void CampaignManager::StartNextStage()
{
	m_currStageIndex++;
	m_currStageFinished = false;
	m_totalSpawnedThisStage = 0;

	m_stageTimer.Reset();
}


//-----------------------------------------------------------------------------------------------
// Returns whether the current stage has finished playing
//
bool CampaignManager::IsCurrentStageFinished() const
{
	return m_currStageFinished;
}


//-----------------------------------------------------------------------------------------------
// Returns the next stage of the manager, for transitions
//
const CampaignStageData* CampaignManager::GetNextStage() const
{
	if (IsCurrentStageFinal())
	{
		return nullptr;
	}

	return &m_campaignDefinition->m_stages[m_currStageIndex + 1];
}


//-----------------------------------------------------------------------------------------------
// Returns the number of enemies alive in the stage still + the number of enemies that still need
// to be spawned in
//
int CampaignManager::GetEnemyCountLeftInStage() const
{
	int total = 0;

	int numEvents = (int)m_currentSpawnEvents.size();

	for (int i = 0; i < numEvents; ++i)
	{
		total += m_currentSpawnEvents[i]->GetEntityCountLeftToSpawn() + m_currentSpawnEvents[i]->GetLiveEntityCount();
	}

	return total;
}


//-----------------------------------------------------------------------------------------------
// Returns whether the current stage is the last stage, for testing victory
//
bool CampaignManager::IsCurrentStageFinal() const
{
	return (m_currStageIndex == (int)m_campaignDefinition->m_stages.size() - 1);
}


//-----------------------------------------------------------------------------------------------
// Returns the total number of entities spawned by this manager
//
int CampaignManager::GetCurrentLiveEnemyCount() const
{
	int numEvents = (int) m_currentSpawnEvents.size();

	int total = 0;
	for (int i = 0; i < numEvents; ++i)
	{
		total += m_currentSpawnEvents[i]->GetLiveEntityCount();
	}

	return total;
}


//-----------------------------------------------------------------------------------------------
// Returns the current stage index
//
int CampaignManager::GetCurrentStageNumber() const
{
	return m_currStageIndex;
}


//-----------------------------------------------------------------------------------------------
// Returns the number of stages currently in the campaign manager
//
int CampaignManager::GetStageCount() const
{
	return (int)m_campaignDefinition->m_stages.size();
}
