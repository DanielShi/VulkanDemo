#pragma once
#include "Predefined.h"
class VertexFormat
{
public:
	enum EVertexElement {
		POSITION,
		NORMAL,
		UV,
		VERTEX_ELEMENT_MAX,
	};

	enum EVertexElementFlag {
		POSITION_BIT				= 1 << POSITION,
		NORMAL_BIT					= 1 << NORMAL,
		UV_BIT						= 1 << UV,
		MASK						= POSITION_BIT | NORMAL_BIT | UV_BIT,
	};

	VertexFormat(uint8_t _flags):m_flags(_flags)
		, m_stride(0)
	{
		Recalculate();
	}

	bool Has( EVertexElement _f ) {
		return (m_flags & (1<<_f)) != 0;
	}	

	void Set( EVertexElement _f ) {
		m_flags |= (1<<_f);
		m_flags &= EVertexElementFlag::MASK;
		Recalculate();
	}

	void Set( uint8_t _flags ) {
		m_flags |= ( _flags & EVertexElementFlag::MASK );
		Recalculate();
	}

	int Offset( EVertexElement _f ) {
		return m_offset[_f];
	}

	int Stride() {
		return m_stride;
	}

	uint8_t Flags() {
		return m_flags;
	}

	void Recalculate() {
		int _element_size[] = {
			sizeof(Vector3),
			sizeof(Vector3),
			sizeof(Vector2),
		};

		for( auto i = 0 ; i < VERTEX_ELEMENT_MAX; ++i ) {
			if (Has(EVertexElement(i))) {
				m_offset[i]	= m_stride;
				m_stride += _element_size[i];
			}
		}
	}
private:

	uint8_t					m_flags;
	int32_t					m_stride;
	int32_t					m_offset[VERTEX_ELEMENT_MAX];
};

