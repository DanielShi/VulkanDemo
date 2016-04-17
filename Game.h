#pragma once
#include <windows.h>
#include "Predefined.h"
#include "tiny_obj_loader.h"
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

	std::vector<tinyobj::shape_t>						m_shapes;
	std::vector<tinyobj::material_t>					m_materials;

};

