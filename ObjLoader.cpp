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
	std::vector<Vector3>	_positions;
	std::vector<Vector3> 	_normals;
	std::vector<Vector2> 	_uvs;
	std::vector<FacePNT>	_faces;

	std::string				_current_obj_name;
	std::string 			_mtl_lib;
	std::string 			_current_mtl;
	char *					_next_token = nullptr;

	std::ifstream _in_file;
	_in_file.open(_path,std::ios::in);

	while (!_in_file.eof()) {
		_in_file.getline(_line, 256);
		//std::getline(_string_stream, _line);			//10 times slow ?
		if (_line[0] == 'v' && _line[1] == 'n') 
			_normals.push_back(ParseNormal(_line));
		else if (_line[0] == 'v' && _line[1] == 't')
			_uvs.push_back(ParseTexcoord(_line));
		else if (_line[0] == 'v')
			_positions.push_back(ParsePosition(_line));
		else if (_line[0] == 'f')
			_faces.push_back(ParseFace(_line));
		else if (_line[0] == 's') {
		}
		else if (_line[0] == 'g') {
			const char * _prefix = strtok_s(_line, " ", &_next_token);
			assert(_prefix);
			_current_obj_name = _next_token;
		}
		else if ( strstr(_line,"mtllib") ) {
			const char* _prefix = strtok_s(_line, " ", &_next_token);
			assert(_prefix);
			_mtl_lib = _next_token;
		}
		else if (strstr(_line, "usemtl")) {
			const char* _prefix = strtok_s(_line, " ", &_next_token);
			assert(_prefix);
			_current_mtl = _next_token;
		}
	}

	_in_file.close();

	return true;
}

Vector3 ObjLoader::ParsePosition(char* _line)
{
	Vector3 _position;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_position.x			= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_position.y			= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_position.z			= atof(_next_token);
	return _position;
}

Vector3 ObjLoader::ParseNormal(char* _line)
{
	Vector3 _normal;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_normal.x			= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_normal.y			= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_normal.z			= atof(_next_token);
	return _normal;
}

Vector2 ObjLoader::ParseTexcoord(char* _line)
{
	Vector2 _uv;
	char* _next_token	= nullptr;
	_line				= strtok_s(_line, " ", &_next_token);
	_uv.u				= atof(_next_token);
	_line				= strtok_s(nullptr, " ", &_next_token);
	_uv.v				= atof(_next_token);
	return _uv;
}

FacePNT ObjLoader::ParseFace(char* _line)
{
	FacePNT _face = {};
	return _face;
}
