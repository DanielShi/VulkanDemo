#include "stdafx.h"
#include "ObjLoader.h"
#include "Mesh.h"
#include "ResourceCache.h"
#include "VertexFormat.h"


ObjLoader::ObjLoader()
{
}


ObjLoader::~ObjLoader()
{
}


bool ObjLoader::Load(const std::string& _path)
{
	char _line[256];

	std::string 			_mtl_lib;
	char *					_next_token = nullptr;

	Group*					_current_group = nullptr;

	std::ifstream _in_file(_path);

	if( !_in_file.good() )
		return false;

	while (!_in_file.eof()) {
		_in_file.getline(_line, 256);
		//std::getline(_string_stream, _line);			//10 times slow ?
		if (_line[0] == 'v' && _line[1] == 'n') 
			m_normals.push_back(ParseVector3(_line));
		else if (_line[0] == 'v' && _line[1] == 't')
			m_uvs.push_back(ParseVector2(_line));
		else if (_line[0] == 'v')
			m_positions.push_back(ParseVector3(_line));
		else if (_line[0] == 'f') {
			assert( _current_group );
			_current_group->faces.push_back(ParseFace(_line));
		}
		else if (_line[0] == 's') {

		}
		else if (_line[0] == 'g') {
			const char * _prefix = strtok_s(_line, " ", &_next_token);
			assert(_prefix);
			Group _new_group;
			_new_group.name = _next_token;
			m_groups.push_back(_new_group);
			_current_group = &m_groups[m_groups.size()-1];
		}
		else if ( strstr(_line,"mtllib") ) {
			const char* _prefix = strtok_s(_line, " ", &_next_token);
			assert(_prefix);
			_mtl_lib = _next_token;
		}
		else if (strstr(_line, "usemtl")) {
			const char* _prefix = strtok_s(_line, " ", &_next_token);
			assert(_prefix);
			assert(_current_group);
			_current_group->material = _next_token;
		}
	}

	_in_file.close();

	if( !_mtl_lib.empty()) {
		if ( !LoadMaterialLib( _mtl_lib ) ) 
			return false;
	}

	return true;
}

void ObjLoader::CreateAll(MeshCache& _meshCache, MaterialCache& _materialCache, TextureCache& _textureCache)
{
	for( auto _g : m_groups ) {
		std::shared_ptr<Mesh> _new_mesh = std::make_shared<Mesh>(_g.name);
		uint8_t _flag = _g.faces[0].vertex[0].flag;
		// count vertex number
		VertexCache _vertex_cache( m_positions, m_normals, m_uvs, _flag );

		for( auto _f : _g.faces ) {
			if (_f.vertex_num == 3) {
				_vertex_cache.AddVertex(_f.vertex[0]);
				_vertex_cache.AddVertex(_f.vertex[1]);
				_vertex_cache.AddVertex(_f.vertex[2]);
			}
			else if (_f.vertex_num == 4) {
				_vertex_cache.AddVertex(_f.vertex[0]);
				_vertex_cache.AddVertex(_f.vertex[1]);
				_vertex_cache.AddVertex(_f.vertex[2]);
				_vertex_cache.AddVertex(_f.vertex[0]);
				_vertex_cache.AddVertex(_f.vertex[2]);
				_vertex_cache.AddVertex(_f.vertex[3]);
			}
			else {
				assert(0&&"unknown vertex format");
			}
		}
		_new_mesh->AllocVertexbuffer(_flag, _vertex_cache.GetVertexCount());
		_new_mesh->AllocIndexBuffer(_vertex_cache.GetIndexCount());

		uint8_t *	_vertex_buffer			= nullptr;
		uint16_t*	_index_buffer			= nullptr;
		int			_vertex_buffer_size		= 0;
		int			_index_buffer_size		= 0;
		int			_vertex_stride			= 0;

		if( _new_mesh->Lock( &_vertex_buffer , &_vertex_buffer_size, &_vertex_stride, &_index_buffer, &_index_buffer_size) )
		{
			memcpy(_vertex_buffer,_vertex_cache.GetVertexBuffer(),_vertex_stride*_vertex_cache.GetVertexCount());
			memcpy(_index_buffer,_vertex_cache.GetIndexBuffer(),_vertex_cache.GetIndexCount()*2);
			_new_mesh->Unlock();
		}

		_meshCache.Add( _new_mesh );
	}
}

Vector3 ObjLoader::ParseVector3(char* _line)
{
	Vector3 _vector;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_vector.x			= static_cast<float>(atof(_next_token));
	_line				= strtok_s(nullptr, " ", &_next_token);
	_vector.y			= static_cast<float>(atof(_next_token));
	_line				= strtok_s(nullptr, " ", &_next_token);
	_vector.z			= static_cast<float>(atof(_next_token));
	return _vector;
}

Vector2 ObjLoader::ParseVector2(char* _line)
{
	Vector2 _vector;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_vector.u			= static_cast<float>(atof(_next_token));
	_line				= strtok_s(nullptr, " ", &_next_token);
	_vector.v			= static_cast<float>(atof(_next_token));
	return _vector;
}

float ObjLoader::ParseFloat(char* _line)
{
	float _float;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_float				= static_cast<float>(atof(_next_token));
	return _float;
}

int ObjLoader::ParseInt(char* _line)
{
	int _int;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_int				= atoi(_next_token);
	return _int;
}

std::string ObjLoader::ParseString(char* _line)
{
	std::string _str;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_str				= _next_token;
	return _str;
}

ObjLoader::Face ObjLoader::ParseFace(char* _line)
{
	Face _face = {};
	char* _next_token	= nullptr;
	// skip command name
	_next_token			= strtok_s(_line, " ", &_line);

	while( _next_token ) {
		_next_token	= strtok_s(_line, " ", &_line);
		if( _next_token ) {
			_face.vertex[_face.vertex_num] = ParseVertex(_next_token);
			_face.vertex_num++;
		}
	}
	return _face;
}

ObjLoader::Vertex ObjLoader::ParseVertex(char* _line)
{
	Vertex _vertex = {};
	char * _next_token	= strtok_s(_line, "/ ", &_line);
	if( _next_token ) {
		if( strlen(_next_token) > 0 ) {
			_vertex.flag		= VertexFormat::EVertexElementFlag::POSITION_BIT;
			_vertex.pos_index   = atoi(_line);
		}
	}
	
	_next_token	= strtok_s(_line, "/ ", &_line);
	if( _next_token ) {
		if( strlen(_next_token) > 0 ) {
			_vertex.flag	|= VertexFormat::EVertexElementFlag::UV_BIT; 
			_vertex.uvw_index = atoi(_next_token);
		}
	}

	_next_token	= strtok_s(_line, "/", &_line);
	if( _next_token ) {
		if( strlen(_next_token) > 0 ) {
			_vertex.flag	|= VertexFormat::EVertexElementFlag::NORMAL_BIT; 
			_vertex.nor_index = atoi(_next_token);
		}
	}
	return _vertex;
}

bool ObjLoader::LoadMaterialLib(const std::string& _mtl_lib)
{
	char _line[256];
	std::ifstream _in_file( _mtl_lib );

	if( !_in_file.good() ) 
		return false;

	Mtl* _current_mtl = nullptr;

	while(!_in_file.eof()){

		_in_file.getline(_line, sizeof(_line));

		if( _line[0] == '#')
			continue;

		if( strlen(_line) == 0 )
			continue;

		if (strstr(_line, "newmtl")) {
			Mtl _new_mtl;
			_new_mtl.flag = 0;
			_new_mtl.name = ParseString(_line);
			m_materials.insert(std::make_pair(_new_mtl.name,_new_mtl));
			_current_mtl = &m_materials[_new_mtl.name];
			continue;
		}
		assert(_current_mtl);
		if (strstr(_line, "map_Ka")) {
			_current_mtl->map_Ka = ParseString(_line);
			_current_mtl->flag |= Mtl::USE_MAP_KA;
			continue;
		}
		if (strstr(_line, "map_Kd")) {
			_current_mtl->map_Kd = ParseString(_line);
			_current_mtl->flag |= Mtl::USE_MAP_KD;
			continue;
		}
		if (strstr(_line, "map_Ks")) {
			_current_mtl->map_Ks = ParseString(_line);
			_current_mtl->flag |= Mtl::USE_MAP_KS;
			continue;
		}
		if (strstr(_line, "map_d")) {
			_current_mtl->map_d = ParseString(_line);
			_current_mtl->flag |= Mtl::USE_MAP_D;
			continue;
		}
		if (strstr(_line, "map_bump") || strstr(_line, "bump") ) {
			_current_mtl->map_bump = ParseString(_line);
			_current_mtl->flag |= Mtl::USE_MAP_BUMP;
			continue;
		}
		if (strstr(_line, "Ns")) {
			_current_mtl->Ns = ParseFloat(_line);
			_current_mtl->flag |= Mtl::USE_NS;
			continue;
		}
		if (strstr(_line, "Ni")) {
			_current_mtl->Ni = ParseFloat(_line);
			_current_mtl->flag |= Mtl::USE_NI;
			continue;
		}
		if (strstr(_line, "Tr")) {
			_current_mtl->Tr = ParseFloat(_line);
			_current_mtl->flag |= Mtl::USE_TR;
			continue;
		}
		if (strstr(_line, "Tf")) {
			_current_mtl->Tf = ParseVector3(_line);
			_current_mtl->flag |= Mtl::USE_TF;
			continue;
		}
		if (strstr(_line, "illum")) {
			_current_mtl->illum = ParseInt(_line);
			_current_mtl->flag |= Mtl::USE_ILLUM;
			continue;
		}
		if (strstr(_line, "Ka")) {
			_current_mtl->Ka = ParseVector3(_line);
			_current_mtl->flag |= Mtl::USE_KA;
			continue;
		}
		if (strstr(_line, "Kd")) {
			_current_mtl->Kd = ParseVector3(_line);
			_current_mtl->flag |= Mtl::USE_KD;
			continue;
		}
		if (strstr(_line, "Ks")) {
			_current_mtl->Ks = ParseVector3(_line);
			_current_mtl->flag |= Mtl::USE_KS;
			continue;
		}
		if (strstr(_line, "Ke")) {
			_current_mtl->Ke = ParseVector3(_line);
			_current_mtl->flag |= Mtl::USE_KE;
			continue;
		}
		if (strstr(_line, "d")) {
			_current_mtl->d = ParseFloat(_line);
			_current_mtl->flag |= Mtl::USE_D;
			continue;
		}
		assert(0);
	}
	_in_file.close();
	return true;
}

VertexCache::VertexCache(const std::vector<Vector3>& _positions, const std::vector<Vector3>& _normals, const std::vector<Vector2>& _uvs, uint8_t _flags):m_positions(_positions)
, m_normals(_normals)
, m_uvs(_uvs)
, m_vertexFormat(_flags)
, m_isVertexBufferDirt(true)
{
}

void VertexCache::AddVertex(const ObjLoader::Vertex& _vertex)
{
	VertexData _vertex_data;
	assert(m_vertexFormat.Flags() ==_vertex.flag); 
	if (m_vertexFormat.Has(VertexFormat::POSITION)) {
		_vertex_data.position	= m_positions[_vertex.pos_index - 1];		//pos_index starts from 1
	}
	if (m_vertexFormat.Has(VertexFormat::NORMAL)) {
		_vertex_data.normal		= m_normals[_vertex.nor_index - 1];
	}
	if (m_vertexFormat.Has(VertexFormat::UV)) {
		_vertex_data.uv			= m_uvs[_vertex.uvw_index - 1];
	}
	auto _it = std::find_if(m_vertexDataList.begin(), m_vertexDataList.end(), [&](const VertexData& _v )->bool {
		return _v.position.x == _vertex_data.position.x
			&& _v.position.y == _vertex_data.position.y
			&& _v.position.z == _vertex_data.position.z
			&& _v.normal.x	 == _vertex_data.normal.x
			&& _v.normal.y	 == _vertex_data.normal.y
			&& _v.normal.z	 == _vertex_data.normal.z
			&& _v.uv.u		 == _vertex_data.uv.u
			&& _v.uv.v		 == _vertex_data.uv.v;
	});
	
	uint16_t _index = UINT16_MAX;	
	if( _it == m_vertexDataList.end()){
		_index = static_cast<uint16_t>(m_vertexDataList.size());
		m_vertexDataList.push_back(_vertex_data);
	} else {
		_index = _it - m_vertexDataList.begin();
	}
	
	m_indexBuffer.push_back(_index);

	m_isVertexBufferDirt = true;
}

int VertexCache::GetVertexCount()
{
	return m_vertexDataList.size();
}

int VertexCache::GetIndexCount()
{
	return m_indexBuffer.size();
}

const uint8_t* VertexCache::GetVertexBuffer()
{
	if ( !m_isVertexBufferDirt )
		return m_vertexBuffer.get();

	m_vertexBuffer.reset( new uint8_t[m_vertexFormat.Stride()*m_vertexDataList.size()] );

	for( auto i = 0u ; i < m_vertexDataList.size(); ++i ) {
		if (m_vertexFormat.Has(VertexFormat::POSITION)) {
			Vector3 * _p = reinterpret_cast<Vector3*>(m_vertexBuffer.get() + m_vertexFormat.Stride()*i + m_vertexFormat.Offset(VertexFormat::POSITION));
			*_p = m_vertexDataList[i].position;
		}
		if (m_vertexFormat.Has(VertexFormat::NORMAL)) {
			Vector3 * _p = reinterpret_cast<Vector3*>(m_vertexBuffer.get() + m_vertexFormat.Stride()*i + m_vertexFormat.Offset(VertexFormat::NORMAL));
			*_p = m_vertexDataList[i].normal;
		}
		if (m_vertexFormat.Has(VertexFormat::UV)) {
			Vector2 * _p = reinterpret_cast<Vector2*>(m_vertexBuffer.get() + m_vertexFormat.Stride()*i + m_vertexFormat.Offset(VertexFormat::UV));
			*_p = m_vertexDataList[i].uv;
		}
	}

	m_isVertexBufferDirt = false;

	return m_vertexBuffer.get();
}

const uint16_t* VertexCache::GetIndexBuffer()
{
	return m_indexBuffer.data();
}
