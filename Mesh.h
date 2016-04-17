#pragma once
#include "Predefined.h"
#include "tiny_obj_loader.h"
#include "UniformBuffer.h"
class Mesh
{
public:
	Mesh(const std::string& _name);
	~Mesh();
	void Create(tinyobj::mesh_t mesh);

protected:
	std::string								m_name;
	UniformBuffer							m_uniformBuffer;
};

