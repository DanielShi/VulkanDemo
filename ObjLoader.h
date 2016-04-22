#pragma once
#include "Predefined.h"
class ObjLoader
{
public:
	ObjLoader();
	~ObjLoader();
	bool									Load(const std::string& _path);
	Vector3									ParsePosition(char* _line);
	Vector3 								ParseNormal(char* _line);
	Vector2 								ParseTexcoord(char* _line);
	FacePNT									ParseFace(char* _line);
};

