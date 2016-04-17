#include "stdafx.h"
#include "Mesh.h"


Mesh::Mesh(const std::string& _name):m_name(_name)
{
}


Mesh::~Mesh()
{
}

void Mesh::Create(tinyobj::mesh_t mesh)
{
	std::vector<float>& _positions = mesh.positions;
	std::vector<float>& _normals = mesh.normals;
	std::vector<float>& _texcoords = mesh.texcoords;
	std::vector<unsigned int>& _indices = mesh.indices;
	VkPrimitiveTopology _topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assert(mesh.num_vertices[0] == 3);
	int _material_id = mesh.material_ids[0];
}
