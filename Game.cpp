#include "stdafx.h"
#include "Game.h"
#include "Scene.h"
#include "VkContext.h"


Game::Game()
{
}


Game::~Game()
{
}


Game* Game::GetInstance()
{
	static Game _instance;
	return &_instance;
}

bool Game::Initialize(HINSTANCE _hinst, HWND _hwnd)
{
	if( !VkContext::GetInstance()->Initialize(_hinst, _hwnd))
		return false;
	//register scenes
	RegisterScene("sponza");

	bool _result = true;
	//load the first scene
	if (!m_scenes.empty()) {
		_result = m_scenes.front()->Load();
		assert(_result);
		m_currentScene = m_scenes.front();
	}
	return _result;
}


void Game::Destroy()
{
	m_scenes.clear();
	VkContext::GetInstance()->Destroy();
}


void Game::Run()
{
	if (m_currentScene) {
		m_currentScene->Display();
	}
	VkContext::GetInstance()->Run();
}

void Game::RegisterScene(const char* _sceneName)
{
	std::shared_ptr<Scene> _new_scene(new Scene(_sceneName));
	m_scenes.push_back(_new_scene);
}
