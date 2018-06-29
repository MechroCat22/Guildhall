/************************************************************************/
/* File: GameState_MainMenu.cpp
/* Author: Andrew Chase
/* Date: March 24th, 2018
/* Description: Implementation of the GameState_MainMenu class
/************************************************************************/
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/GameStates/GameState_Ready.hpp"
#include "Game/GameStates/GameState_Playing.hpp"
#include "Game/GameStates/GameState_MainMenu.hpp"

#include "Engine/Core/Window.hpp"
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Rendering/Core/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"

//-----------------------------------------------------------------------------------------------
// Default constructor
//
GameState_MainMenu::GameState_MainMenu()
	: m_cursorPosition(0)
{
	float aspect = Window::GetInstance()->GetAspect();
	float height = Renderer::UI_ORTHO_HEIGHT;

	m_menuBounds = AABB2(Vector2(0.25f * aspect * height, 0.25f * height), Vector2(0.75f * aspect * height, 0.75f * height));
	m_fontHeight = 100.f;

	m_menuOptions.push_back("Play");
	m_menuOptions.push_back("Quit");
}


//-----------------------------------------------------------------------------------------------
// Checks for input related to this GameState and changes state accordingly
//
void GameState_MainMenu::ProcessInput()
{
	InputSystem* input = InputSystem::GetInstance();

	// Moving down
	bool keyPressedDown = input->WasKeyJustPressed(InputSystem::KEYBOARD_DOWN_ARROW);
	if (keyPressedDown)
	{
		m_cursorPosition++;
		if (m_cursorPosition > (int) (m_menuOptions.size()) - 1)
		{
			m_cursorPosition = 0;
		}
	}

	// Moving up
	bool keyPressedUp = input->WasKeyJustPressed(InputSystem::KEYBOARD_UP_ARROW);
	if (keyPressedUp)
	{
		m_cursorPosition--;
		if (m_cursorPosition < 0)
		{
			m_cursorPosition = (int) (m_menuOptions.size()) - 1;
		}
	}

	// Selection
	if (input->WasKeyJustPressed(InputSystem::KEYBOARD_SPACEBAR))
	{
		ProcessMenuSelection();
	}

	// Quit
	if (input->WasKeyJustPressed(InputSystem::KEYBOARD_ESCAPE))
	{
		App::GetInstance()->Quit();
	}
}


//-----------------------------------------------------------------------------------------------
// Updates the MainMenu state
//
void GameState_MainMenu::Update()
{
}


//-----------------------------------------------------------------------------------------------
// Renders the Main Menu
//
void GameState_MainMenu::Render() const
{
	Renderer* renderer = Renderer::GetInstance();
	renderer->SetCurrentCamera(renderer->GetUICamera());

	// Setup the renderer
	renderer->ClearScreen(Rgba::LIGHT_BLUE);
	renderer->Draw2DQuad(m_menuBounds, AABB2::UNIT_SQUARE_OFFCENTER, Rgba::BLUE, AssetDB::GetSharedMaterial("UI"));

	// Draw the menu options
	BitmapFont* font = AssetDB::CreateOrGetBitmapFont("Data/Images/Fonts/Default.png");
	AABB2 currentTextBounds = m_menuBounds;

	for (int menuIndex = 0; menuIndex < static_cast<int>(m_menuOptions.size()); ++menuIndex)
	{
		Rgba color = Rgba::WHITE;
		if (menuIndex == m_cursorPosition)
		{
			color = Rgba::YELLOW;
		}
		renderer->DrawTextInBox2D(m_menuOptions[menuIndex].c_str(), currentTextBounds, Vector2(0.5f, 0.5f), m_fontHeight, TEXT_DRAW_SHRINK_TO_FIT, font, color);
		currentTextBounds.Translate(Vector2(0.f, -m_fontHeight));
	}
}


//-----------------------------------------------------------------------------------------------
// Called when the game transitions into this state, before the first update
//
void GameState_MainMenu::Enter()
{
	AudioSystem* audio = AudioSystem::GetInstance();
	SoundID sound = audio->CreateOrGetSound("Data/Audio/Music/MainMenu.mp3");

	m_mainMenuMusic = AudioSystem::GetInstance()->PlaySound(sound, true);
}


//-----------------------------------------------------------------------------------------------
// Called when the game transitions out of this state, before deletion
//
void GameState_MainMenu::Leave()
{
	AudioSystem* audio = AudioSystem::GetInstance();
	audio->StopSound(m_mainMenuMusic);
}


//-----------------------------------------------------------------------------------------------
// Processes the enter command on a menu selection
//
void GameState_MainMenu::ProcessMenuSelection() const
{
	std::string selectedOption = m_menuOptions[m_cursorPosition];

	if (selectedOption == "Play")
	{
		Game::TransitionToGameState(new GameState_Ready());
	}
	else if (selectedOption == "Quit")
	{
		App::GetInstance()->Quit();
	}
}
