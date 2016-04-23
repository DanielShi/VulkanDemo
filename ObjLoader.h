#pragma once
#include "Predefined.h"
#include "VertexFormat.h"
#include "ResourceCache.h"

class Mesh;

class ObjLoader
{
public:
	ObjLoader();
	~ObjLoader();
	
	//Face with postion, normal, uv
	struct Vertex {
		int						pos_index;
		int						nor_index;
		int						uvw_index;
		uint8_t					flag;
	};

	struct Face {
		static const int		MAX_VERTEX = 6;
		Vertex					vertex[MAX_VERTEX];
		int						vertex_num;
	};

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
		std::string			name;
	};

	bool									Load(const std::string& _path);
	void									CreateAll( MeshCache& _meshCache, MaterialCache& _materialCache, TextureCache& _textureCache );

protected:
	Vector3									ParseVector3(char* _line);
	Vector2 								ParseVector2(char* _line);
	float									ParseFloat(char* _line);
	int										ParseInt(char* _line);
	std::string								ParseString(char* _line);
	Face									ParseFace(char* _line);
	Vertex									ParseVertex(char* _line);
	bool									LoadMaterialLib(const std::string& _mtl_lib);
	
	std::vector<Vector3>					m_positions;
	std::vector<Vector3> 					m_normals;
	std::vector<Vector2> 					m_uvs;
	std::vector<Group>						m_groups;
	std::map<std::string, Mtl>				m_materials;
	
};

class VertexCache {
public:
	VertexCache( const std::vector<Vector3>& _positions, const std::vector<Vector3>& _normals, const std::vector<Vector2>& _uvs, uint8_t _flags);
	void									AddVertex(const ObjLoader::Vertex& _vertex);
	int										GetVertexCount();
	int										GetIndexCount();
	const uint8_t*							GetVertexBuffer();
	const uint16_t*							GetIndexBuffer();
private:
	struct VertexData {
		Vector3			position;
		Vector3			normal;
		Vector2			uv;
	};

	const std::vector<Vector3>& 			m_positions;
	const std::vector<Vector3>& 			m_normals;
	const std::vector<Vector2>& 			m_uvs;
	std::vector<VertexData>					m_vertexDataList;
	std::vector<uint16_t>					m_indexBuffer;
	std::unique_ptr<uint8_t[]>				m_vertexBuffer;
	bool									m_isVertexBufferDirt;
	VertexFormat							m_vertexFormat;
};