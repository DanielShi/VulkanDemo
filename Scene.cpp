#include "stdafx.h"
#include "Scene.h"
#include "tiny_obj_loader.h"
#include "Mesh.h"


Scene::Scene( const char * _name ) : m_name( _name )
, m_isLoaded(false)
{
}


Scene::~Scene()
{
}


bool Scene::Load()
{
	std::string _path = m_name + ".obj";

	std::string							_errors;
	if (!tinyobj::LoadObj(m_shapes, m_materials, _errors, _path.c_str())) {
		assert(false && _errors.c_str());
		return false;
	}

	for (auto _shape : m_shapes) {
		std::shared_ptr<Mesh> _new_mesh(new Mesh(_shape.name));
		_new_mesh->Create(_shape.mesh);
		m_meshes.push_back(_new_mesh);
	}

	m_isLoaded = true;
	return true;
}


void Scene::Display()
{
}
