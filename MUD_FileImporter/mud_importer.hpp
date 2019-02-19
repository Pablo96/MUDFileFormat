#pragma once
#include <vector>
#include <GLM/glm.hpp>
#include <GLM/gtx/quaternion.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include "mud_importer.hpp"

namespace MUDLoader
{

	typedef glm::mat4 mat4;
	typedef glm::vec2 vec2;
	typedef glm::vec3 vec3;
	typedef glm::vec4 vec4;
	typedef glm::uvec4 vec4u;
	typedef glm::quat quat;
	typedef float decimal;

	void quaternionToMatrix(quat& q, mat4& mat);

	void translationToMatrix(vec3& vec, mat4& mat);

	mat4 inverseMat4(const mat4& matrix);

	template<class A, class B, class C>
	struct tuple
	{
		A parentID;
		B first;
		C second;
	};

	#define MAX_VERTEX_BONES 4	// Max number of bones per vertex
	#define PRECISION 12
	
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
		vec2 uvCoord = vec2(0);
		vec4u indices = vec4u(0); //Bones indices. Up to 4 bones
		vec4 weights = vec4(0);	//Bones weight. 1 per bone (4 total)
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
		std::vector<tuple<int, mat4*, mat4*>> bindTransforms;
	};

	void LoadASCII(const char* filePath, Model** model);

	void LoadBinary(const char* filePath, Model** model);
}