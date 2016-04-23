#pragma once
#include "Predefined.h"
#include "Mesh.h"
#include "ResourceCache.h"

class Scene
{
public:
	Scene(const char* _name);
	~Scene();
	bool							Load();
	void							Display();
protected:
	std::string							m_name;
	bool								m_isLoaded;
	MeshCache							m_meshCache;
	MaterialCache						m_materialCache;
	TextureCache						m_textureCache;
};

