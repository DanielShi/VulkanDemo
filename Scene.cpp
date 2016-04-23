#include "stdafx.h"
#include "Scene.h"
#include "Mesh.h"
#include "ObjLoader.h"


Scene::Scene(const char * _name) : m_name(_name)
, m_isLoaded(false)
{
}


Scene::~Scene()
{
}


bool Scene::Load()
{
	std::string _path = m_name + ".obj";
	std::unique_ptr<ObjLoader> _loader = std::make_unique<ObjLoader>();
	if( !_loader->Load(_path) ) {
		return false;
	}
	_loader->CreateAll(m_meshCache,m_materialCache,m_textureCache);
	m_isLoaded = true;
	return true;
}


void Scene::Display()
{
}
