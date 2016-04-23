#pragma once
#include <windows.h>
#include "Predefined.h"
class Scene;
class Game
{
public:
	Game();
	~Game();
	static Game* GetInstance();
	bool Initialize(HINSTANCE _hinst, HWND _hwnd);
	void Destroy();
	void Run();

protected:
	void RegisterScene(const char* _sceneName);
	std::vector<std::shared_ptr<Scene>>					m_scenes;
	std::shared_ptr<Scene>								m_currentScene;
};

