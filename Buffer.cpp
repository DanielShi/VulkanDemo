#include "stdafx.h"
#include "Buffer.h"


Buffer::Buffer() : m_size(0)
{
}


Buffer::~Buffer()
{
}

void Buffer::Alloc(int _size)
{
	m_size = _size;
	m_buffer.reset( new uint8_t[_size]);
}


void Buffer::Unlock()
{
}
