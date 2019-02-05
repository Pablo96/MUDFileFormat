#pragma once
#include <vector>
#include <utility> //Pair

namespace MUDLoader
{
	#define MAX_VERTEX_BONES 4	// Max number of bones per vertex
#ifdef MUD_IMPORTER_HIGHP
	typedef double decimal;
#define PRECISION 20
#else
	typedef float decimal;
	#define PRECISION 10
#endif

	struct vec2
	{
		decimal x, y;
	};

	struct vec3
	{
		decimal x, y, z;
	};

	struct vec4
	{
		decimal x, y, z, w;
	
		vec4 operator*(const vec4& rhs) const
		{
			vec4 result;
			result.x = this->x * rhs.x;
			result.y = this->y * rhs.y;
			result.z = this->z * rhs.z;
			result.w = this->w * rhs.w;
			return result;
		}

		vec4 operator-(const vec4& rhs) const
		{
			vec4 result;
			result.x = this->x - rhs.x;
			result.y = this->y - rhs.y;
			result.z = this->z - rhs.z;
			result.w = this->w - rhs.w;
			return result;
		}

		vec4 operator+(const vec4& rhs) const
		{
			vec4 result;
			result.x = this->x + rhs.x;
			result.y = this->y + rhs.y;
			result.z = this->z + rhs.z;
			result.w = this->w + rhs.w;
			return result;
		}
		
	};

	struct vec4u
	{
		unsigned int x, y, z, w;
	};
	


	typedef vec4 quatd;

	struct mat4
	{	
		decimal xx, yx, zx, wx;
		decimal xy, yy, zy, wy;
		decimal xz, yz, zz, wz;
		decimal xw, yw, zw, ww;

		mat4()
		{
		}

		mat4(const vec4& vec1, const vec4& vec2, const vec4& vec3, const vec4& vec4)
		{
			xx = vec1.x; yx = vec1.y; zx = vec1.z; wx = vec1.w;
			xy = vec2.x; yy = vec2.y; zy = vec2.z; wy = vec2.w;
			xz = vec3.x; yz = vec3.y; zz = vec3.z; wz = vec3.w;
			xw = vec4.x; yw = vec4.y; zw = vec4.z; ww = vec4.w;
		}
		
		vec4 operator[](const int index) const
		{
			switch (index)
			{
			case 0:
				return vec4({ xx, yx, zx, wx });
			case 1:
				return vec4({ xy, yx, zx, wx });
			case  2:
				return vec4({ xz, yz, zz, wz });
			case 3: 
				return vec4({ xw, yw, zw, ww });
			default:
				return vec4({ 0 });
			}
		}

		mat4 operator*(const decimal rhs) const
		{
			mat4 result;

			result.xx = this->xx * rhs;
			result.xy = this->xy * rhs;
			result.xz = this->xz * rhs;
			result.xw = this->xw * rhs;
						
			result.yx = this->yx * rhs;
			result.yy = this->yy * rhs;
			result.yz = this->yz * rhs;
			result.yw = this->yw * rhs;
						
			result.zx = this->zx * rhs;
			result.zy = this->zy * rhs;
			result.zz = this->zz * rhs;
			result.zw = this->zw * rhs;
						
			result.wx = this->wx * rhs;
			result.wy = this->wy * rhs;
			result.wz = this->wz * rhs;
			result.ww = this->ww * rhs;

			return result;
		}
	};


	struct Bone
	{
		int id;
		Bone* parent;
		mat4 bindOffset;	// Original offset relative to the parent
		mat4 inverseBindOffset;
		std::vector<Bone*> children;
		const char* debugName;
	};

	//###################################################################
	//				VERTEX, MESH AND MODEL
	//###################################################################

	struct Vertex
	{
		vec3 pos;
		vec3 normal;
		vec2 uvCoord = { 0 };
		vec4u indices = { 0 }; //Bones indices. Up to 4 bones
		vec4 weights = { 0 };	//Bones weight. 1 per bone (4 total)
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
	};

	struct Model
	{
		std::vector<Mesh> meshes;
		Bone* skeleton = nullptr;
		std::vector<std::pair<mat4*, mat4*>> bindTransforms;
	};

	void LoadASCII(const char* filePath, Model** model);

	void LoadBinary(const char* filePath, Model** model);
}