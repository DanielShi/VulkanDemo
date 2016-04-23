#include "stdafx.h"
#include "Mesh.h"
#include "VertexFormat.h"


Mesh::Mesh(const std::string& _name):m_name(_name)
,m_materialId(UINT_MAX)
,m_vertexFormat(0)
{
}


Mesh::~Mesh()
{

}

void Mesh::AllocVertexbuffer(uint8_t _flags, int _vertex_count)
{
	m_vertexFormat.Set(_flags);
	m_vertexBuffer.Alloc(m_vertexFormat.Stride()*_vertex_count);
}

void Mesh::AllocIndexBuffer(uint32_t _count)
{
	m_indexBuffer.Alloc(_count*sizeof(uint16_t));
}

bool Mesh::Lock(uint8_t ** _vertex_buffer, int* _vertex_buffer_size, int* _vertex_stride, uint16_t** _index_buffer, int* _index_buffer_size)
{
	if(_vertex_stride)
		*_vertex_stride = m_vertexFormat.Stride();

	if (!m_vertexBuffer.Lock(_vertex_buffer,_vertex_buffer_size))
		return false;
	if( !m_indexBuffer.Lock(_index_buffer,_index_buffer_size))
		return false;
	return true;
}

void Mesh::Unlock()
{
	m_vertexBuffer.Unlock();
	m_indexBuffer.Unlock();
}

