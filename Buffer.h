#pragma once
#include "Predefined.h"
class Buffer
{
public:
	Buffer();
	~Buffer();
	void Alloc(int _size);
	template<class T> bool Lock(T ** _buffer, int* _buffer_size);
	void Unlock();

	std::unique_ptr<uint8_t[]>				m_buffer;
	int										m_size;
};

template<class T>
bool Buffer::Lock(T ** _buffer, int* _buffer_size)
{
	if( m_size == 0 ) return false;
	
	if( !m_buffer ) return false;

	if( _buffer_size ){ *_buffer_size = m_size; }

	if( _buffer ) {
		*_buffer = reinterpret_cast<T*>( m_buffer.get());
	}
	return true;
}
