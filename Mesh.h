#pragma once
#include "Predefined.h"
#include "UniformBuffer.h"
#include "Buffer.h"
#include "VertexFormat.h"

class Mesh
{
public:
	Mesh(const std::string& _name);
	~Mesh();
	void AllocVertexbuffer(uint8_t _flags, int _vertex_count);
	void AllocIndexBuffer(uint32_t _count);
	bool Lock(uint8_t ** _vertex_buffer, int* _vertex_buffer_size, int* _vertex_stride, uint16_t** _index_buffer, int* _index_buffer_size);
	void Unlock();
protected:
	std::string								m_name;
	Buffer									m_vertexBuffer;
	Buffer									m_indexBuffer;
	uint32_t								m_materialId;
	VertexFormat							m_vertexFormat;
};

