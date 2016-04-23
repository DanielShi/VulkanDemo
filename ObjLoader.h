#pragma once
#include "Predefined.h"
class ObjLoader
{
public:
	ObjLoader();
	~ObjLoader();
	
	struct Group {
		std::string			name;
		std::string			material;
		std::vector<Face>	faces;
	};

	struct Mtl {
		enum {
			USE_NS			= 1,
			USE_NI			= 1 << 1,
			USE_D			= 1 << 2,
			USE_TR			= 1 << 3,
			USE_TF			= 1 << 4,
			USE_ILLUM		= 1 << 5,
			USE_KA			= 1 << 6,
			USE_KD			= 1 << 7,
			USE_KS			= 1 << 8,
			USE_KE			= 1 << 9,
			USE_MAP_KA		= 1 << 10,
			USE_MAP_KD		= 1 << 11,
			USE_MAP_KS		= 1 << 12,
			USE_MAP_D		= 1 << 13,
			USE_MAP_BUMP	= 1 << 14,
			USE_MASK		= 0x7fff,
		};

		float				Ns;
		float				Ni;
		float				d;
		float				Tr;
		Vector3				Tf;
		int					illum;
		Vector3				Ka;
		Vector3				Kd;
		Vector3				Ks;
		Vector3				Ke;
		std::string			map_Ka;
		std::string			map_Kd;
		std::string			map_Ks;
		std::string			map_d;
		std::string			map_bump;
		unsigned int		flag;
	};

	bool									Load(const std::string& _path);

protected:
	Vector3									ParseVector3(char* _line);
	Vector2 								ParseVector2(char* _line);
	float									ParseFloat(char* _line);
	int										ParseInt(char* _line);
	std::string								ParseString(char* _line);
	Face									ParseFace(char* _line);
	Vertex									ParseVertex(char* _line);
	bool									LoadMaterialLib(const std::string& _mtl_lib);
	void									Trianglized( Face* _face );
	
	std::vector<Vector3>					m_positions;
	std::vector<Vector3> 					m_normals;
	std::vector<Vector2> 					m_uvs;
	std::vector<Group>						m_groups;
	std::vector<Mtl>						m_materials;
	
};

