#pragma once
#include "Predefined.h"
#include "Mesh.h"
#include "tiny_obj_loader.h"
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
	std::vector<std::shared_ptr<Mesh>>	m_meshes;
	std::vector<tinyobj::shape_t>		m_shapes;
	std::vector<tinyobj::material_t>	m_materials;

	VkCommandBuffer						m_commandBuffer;
	VkPipeline							m_Pipeline;
	VkPipelineLayout					m_PipeLayout;
	VkDescriptorSet						m_DescriptorSet;
};

