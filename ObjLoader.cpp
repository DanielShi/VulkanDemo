#include "stdafx.h"
#include "ObjLoader.h"


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

Vector3 ObjLoader::ParseVector3(char* _line)
{
	Vector3 _vector;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_vector.x			= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_vector.y			= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_vector.z			= atof(_next_token);
	return _vector;
}

Vector2 ObjLoader::ParseVector2(char* _line)
{
	Vector2 _vector;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_vector.u			= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_vector.v			= atof(_next_token);
	return _vector;
}

float ObjLoader::ParseFloat(char* _line)
{
	float _float;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_float				= atof(_next_token);
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

Face ObjLoader::ParseFace(char* _line)
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

	Trianglized(&_face);

	return _face;
}

Vertex ObjLoader::ParseVertex(char* _line)
{
	Vertex _vertex = {};
	char * _next_token	= strtok_s(_line, "/ ", &_line);
	if( _next_token ) {
		if( strlen(_next_token) > 0 ) {
			_vertex.flag		= EVertexElementFlag::POSITION;
			_vertex.pos_index   = atoi(_line);
		}
	}
	
	_next_token	= strtok_s(_line, "/ ", &_line);
	if( _next_token ) {
		if( strlen(_next_token) > 0 ) {
			_vertex.flag	|= EVertexElementFlag::UV; 
			_vertex.uvw_index = atoi(_next_token);
		}
	}

	_next_token	= strtok_s(_line, "/", &_line);
	if( _next_token ) {
		if( strlen(_next_token) > 0 ) {
			_vertex.flag	|= EVertexElementFlag::NORMAL; 
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
		std::cout << _line << std::endl;

		if( _line[0] == '#')
			continue;

		if( strlen(_line) == 0 )
			continue;

		if (strstr(_line, "newmtl")) {
			Mtl _new_mtl;
			_new_mtl.flag = 0;
			m_materials.push_back(_new_mtl);
			_current_mtl = &m_materials[m_materials.size() - 1];
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

void ObjLoader::Trianglized(Face* _face)
{
	if( _face ) {
		if( _face->vertex_num == 3 ) 
			return;
		if( _face->vertex_num == 6 )
			return;
		if( _face->vertex_num == 4 ) {
			// 0, 1, 2
			// 2, 1, 3
			_face->vertex[5] = _face->vertex[3];
			_face->vertex[3] = _face->vertex[2];
			_face->vertex[4] = _face->vertex[1];
			_face->vertex_num = 6;
			return;
		}
		assert(0);
	}
}
