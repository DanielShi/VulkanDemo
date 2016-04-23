#pragma once
#include "Predefined.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"

template<class T>
class ResourceCache
{
public:
	ResourceCache();
	~ResourceCache();
	void Add(std::shared_ptr<T> _new_resource);

protected:
	std::vector<std::shared_ptr<T>>		m_resources;
};

template<class T>
ResourceCache<T>::ResourceCache()
{
}

template<class T>
ResourceCache<T>::~ResourceCache()
{
}

template<class T>
void ResourceCache<T>::Add(std::shared_ptr<T> _new_resource)
{
	m_resources.push_back(_new_resource);
}

using MeshCache			= ResourceCache<Mesh>;
using MaterialCache		= ResourceCache<Material>;
using TextureCache		= ResourceCache<Texture>;