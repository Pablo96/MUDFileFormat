#include "tinyxml2.h"
#include "mud_importer.hpp"
#include <sstream>
#include <iostream>
#include <iterator>   // istream_iterator
#include <algorithm> // sort

void quaternionToMatrix(MUDLoader::quatd& q, MUDLoader::mat4& mat)
{
	mat.xx = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
	mat.xy = 2 * q.x * q.y - 2 * q.z * q.w;
	mat.xz = 2 * q.x * q.z + 2 * q.y * q.w;

	mat.yx = 2 * q.x * q.y + 2 * q.z * q.w;
	mat.yy = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
	mat.yz = 2 * q.y * q.z - 2 * q.x * q.w;

	mat.zx = 2 * q.x * q.z - 2 * q.y * q.w;
	mat.zy = 2 * q.y * q.z + 2 * q.x * q.w;
	mat.zz = 1 - 2 * q.x * q.x - 2 * q.y * q.y;

	mat.wx = 0;
	mat.wy = 0;
	mat.wz = 0;
	mat.ww = 1;
}

void translationToMatrix(MUDLoader::vec3& vec, MUDLoader::mat4& mat)
{
	mat.xw = vec.x;
	mat.yw = vec.y;
	mat.zw = vec.z;
}

MUDLoader::mat4 inverseMat4(const MUDLoader::mat4& matrix)
{
	using namespace MUDLoader;

	decimal Coef00 = matrix.zz * matrix.ww - matrix.wz * matrix.zw;
	decimal Coef02 = matrix.yz * matrix.ww - matrix.wz * matrix.yw;
	decimal Coef03 = matrix.yz * matrix.zw - matrix.zz * matrix.yw;
	
	decimal Coef04 = matrix.zy * matrix.ww - matrix.wy * matrix.zw;
	decimal Coef06 = matrix.yy * matrix.ww - matrix.wy * matrix.yw;
	decimal Coef07 = matrix.yy * matrix.zw - matrix.zy * matrix.yw;
	
	decimal Coef08 = matrix.zy * matrix.wz - matrix.wy * matrix.zz;
	decimal Coef10 = matrix.yy * matrix.wz - matrix.wy * matrix.yz;
	decimal Coef11 = matrix.yy * matrix.zz - matrix.zy * matrix.yz;
	
	decimal Coef12 = matrix.zx * matrix.ww - matrix.wx * matrix.zw;
	decimal Coef14 = matrix.yx * matrix.ww - matrix.wx * matrix.yw;
	decimal Coef15 = matrix.yx * matrix.zw - matrix.zx * matrix.yw;
	
	decimal Coef16 = matrix.zx * matrix.wz - matrix.wx * matrix.zz;
	decimal Coef18 = matrix.yx * matrix.wz - matrix.wx * matrix.yz;
	decimal Coef19 = matrix.yx * matrix.zz - matrix.zx * matrix.yz;
	
	decimal Coef20 = matrix.zx * matrix.wy - matrix.wx * matrix.zy;
	decimal Coef22 = matrix.yx * matrix.wy - matrix.wx * matrix.yy;
	decimal Coef23 = matrix.yx * matrix.zy - matrix.zx * matrix.yy;

	vec4 Fac0 = {Coef00, Coef00, Coef02, Coef03};
	vec4 Fac1 = {Coef04, Coef04, Coef06, Coef07};
	vec4 Fac2 = {Coef08, Coef08, Coef10, Coef11};
	vec4 Fac3 = {Coef12, Coef12, Coef14, Coef15};
	vec4 Fac4 = {Coef16, Coef16, Coef18, Coef19};
	vec4 Fac5 = {Coef20, Coef20, Coef22, Coef23};

	vec4 Vec0 = {matrix.yx, matrix.xx, matrix.xx, matrix.xx};
	vec4 Vec1 = {matrix.yy, matrix.xy, matrix.xy, matrix.xy};
	vec4 Vec2 = {matrix.yz, matrix.xz, matrix.xz, matrix.xz};
	vec4 Vec3 = {matrix.yw, matrix.xw, matrix.xw, matrix.xw};

	vec4 Inv0 = {Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2};
	vec4 Inv1 = {Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4};
	vec4 Inv2 = {Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5};
	vec4 Inv3 = {Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5};

	vec4 SignA = {+1, -1, +1, -1};
	vec4 SignB = {-1, +1, -1, +1};
	mat4 Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

	vec4 Row0;
	Row0.x = Inverse[0].x;
	Row0.y = Inverse[1].x;
	Row0.z = Inverse[2].x;
	Row0.w = Inverse[3].x;

	vec4 Dot0(matrix[0] * Row0);
	decimal Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

	decimal OneOverDeterminant = 1 / Dot1;

	return Inverse * OneOverDeterminant;
}


void helperStrToVec3(const char* charArray, MUDLoader::vec3& vec)
{
	std::string string(charArray);
	
	size_t first = string.find_first_of(", ");

	auto sub = string.substr(0, PRECISION);

	// X
	{
		size_t first = sub.find_first_of(", ");
		if (first != std::string::npos)
			sub = sub.replace(first, std::string::npos, "");
		vec.x = (MUDLoader::decimal) atof(sub.c_str());
	}

	// Y
	{
		first = string.find(", ", first) + 2;
		sub = string.substr(first, PRECISION);

		size_t first = sub.find_first_of(", ");
		if (first != std::string::npos)
			sub = sub.replace(first, std::string::npos, "");

		vec.y = (MUDLoader::decimal) atof(sub.c_str());
	}

	// Z
	{
		first = string.find(", ", first) + 2;
		sub = string.substr(first, PRECISION);

		vec.z = (MUDLoader::decimal) atof(sub.c_str());
	}
}

void helperStrToVec4(const char* charArray, MUDLoader::vec4& vec)
{
	std::string string(charArray);

	size_t first = string.find_first_of(", ");

	auto sub = string.substr(0, PRECISION);

	// X
	{
		size_t first = sub.find_first_of(", ");
		if (first != std::string::npos)
			sub = sub.replace(first, std::string::npos, "");
		vec.x = (MUDLoader::decimal) atof(sub.c_str());
	}

	// Y
	{
		first = string.find(", ", first) + 2;
		sub = string.substr(first, PRECISION);
		
		size_t first = sub.find_first_of(", ");
		if (first != std::string::npos)
			sub = sub.replace(first, std::string::npos, "");
		
		vec.y = (MUDLoader::decimal) atof(sub.c_str());
	}

	// Z
	{
		first = string.find(", ", first) + 2;
		sub = string.substr(first, PRECISION);
		
		
		size_t first = sub.find_first_of(", ");
		if (first != std::string::npos)
			sub = sub.replace(first, std::string::npos, "");

		vec.z = (MUDLoader::decimal) atof(sub.c_str());
	}

	// W
	{
		first = string.find(", ", first) + 2;
		sub = string.substr(first, PRECISION);

		vec.w = (MUDLoader::decimal) atof(sub.c_str());
	}
}


void helperBoneBuild(MUDLoader::Bone& bone, tinyxml2::XMLElement* boneNode)
{
	auto translation = boneNode->FindAttribute("translation")->Value();
	auto rotation = boneNode->FindAttribute("rotation")->Value();
	auto name = boneNode->FindAttribute("name")->Value();
	auto id = boneNode->FindAttribute("id")->Value();
	bone.debugName = name;
	bone.id = atoi(id);

	MUDLoader::vec3 translationVec = MUDLoader::vec3();
	helperStrToVec3(translation, translationVec);

	MUDLoader::quatd rotationQuat = MUDLoader::quatd();
	helperStrToVec4(rotation, rotationQuat);

	quaternionToMatrix(rotationQuat, bone.bindOffset);
	translationToMatrix(translationVec, bone.bindOffset);

	bone.inverseBindOffset = inverseMat4(bone.bindOffset);
}

void skeletonBuild(tinyxml2::XMLElement* node, MUDLoader::Bone* parent, std::vector<MUDLoader::Bone*>& array)
{
	for (auto siblingNode = node; siblingNode != nullptr; siblingNode = siblingNode->NextSiblingElement("bone"))
	{
		MUDLoader::Bone* boneTmp = new MUDLoader::Bone();
		helperBoneBuild(*boneTmp, siblingNode);
		
		// double linking
		parent->children.emplace_back(boneTmp);
		boneTmp->parent = parent;

		// Matrix array
		array.emplace_back(boneTmp);

		skeletonBuild(siblingNode->FirstChildElement("bone"), boneTmp, array);
	}
}

void MUDLoader::LoadASCII(const char * filePath, Model** model)
{
	tinyxml2::XMLDocument doc;
	auto loaded = doc.LoadFile(filePath);

	if (loaded == tinyxml2::XMLError::XML_ERROR_FILE_NOT_FOUND)
	{
		std::cout << "FILE NOT FOUND: " << filePath << std::endl;
		return;
	}

	tinyxml2::XMLElement* modelNode = doc.FirstChildElement("model");

	if (!modelNode)
	{
		std::cout << "FILE NOT A MUD MODEL: " << filePath << std::endl;
		return;
	}

	std::vector<Mesh> meshes;

	// Meshes of the model
	for (auto meshNode = modelNode->FirstChildElement("mesh");
		meshNode != nullptr;
		meshNode = meshNode->NextSiblingElement("mesh"))
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		// Mesh vertices
		for (auto vertexNode = meshNode->FirstChildElement("vertex");
			vertexNode != nullptr;
			vertexNode = vertexNode->NextSiblingElement("vertex"))
		{
			Vertex vertex;

			// Position
			auto text = vertexNode->FirstChildElement("position")->FirstAttribute()->Value();
			{
				std::stringstream string(text);
				string >> vertex.pos.x;
				string >> vertex.pos.y;
				string >> vertex.pos.z;
			}

			// Normal
			text = vertexNode->FirstChildElement("normal")->FirstAttribute()->Value();
			{
				std::stringstream string(text);
				string >> vertex.normal.x;
				string >> vertex.normal.y;
				string >> vertex.normal.z;
			}

			// Bone Indices
			auto hasIndices = vertexNode->FirstChildElement("indices");
			if (hasIndices)
			{
				text = vertexNode->FirstChildElement("indices")->FirstAttribute()->Value();
				{
					std::stringstream string(text);
					string >> vertex.indices.x;
					string >> vertex.indices.y;
					string >> vertex.indices.z;
					string >> vertex.indices.w;
				}
				// Bone Weights
				text = vertexNode->FirstChildElement("weights")->FirstAttribute()->Value();
				{
					std::stringstream string(text);
					string >> vertex.weights.x;
					string >> vertex.weights.y;
					string >> vertex.weights.z;
					string >> vertex.weights.w;
				}
			}
			vertices.emplace_back(vertex);
		}

		// Vertex indices
		tinyxml2::XMLElement* indicesNode = meshNode->FirstChildElement("indices");
		unsigned int count = std::strtoul(indicesNode->FirstAttribute()->Value(), nullptr, 0);
		auto text = indicesNode->FindAttribute("values")->Value();
		std::stringstream string(text);
		unsigned int index;
		for (size_t i = 0; i < count; i++)
		{
			string >> index;
			indices.emplace_back(index);
		}

		meshes.emplace_back(Mesh({ vertices, indices }));
	}

	// Skeleton of the model
	bool init = true;
	tinyxml2::XMLElement* skeletonNode = modelNode->FirstChildElement("skeleton");
	
	Bone* skeleton = nullptr;
	std::vector<std::pair<mat4*, mat4*>> transformsArray;

	if (skeletonNode)
	{
		// ID sorted bone array
		std::vector<Bone*> bonesArray;

		// CREATE the root bone (skeleton must have a unique root bone!)
		skeleton = new Bone();
		auto rootNode = skeletonNode->FirstChildElement("bone");
		helperBoneBuild(*skeleton, rootNode);
		bonesArray.emplace_back(skeleton);
		
		// BUILD skeleton and bone array (should be sorted)
		skeletonBuild(rootNode->FirstChildElement("bone"), skeleton, bonesArray);

		// SORT in case
		std::sort(bonesArray.rbegin(), bonesArray.rend(), [](Bone* bone1, Bone* bone2)
		{
			return bone1->id > bone2->id;
		});

		// BUILD transforms array
		for (auto bone : bonesArray)
		{
			std::pair<mat4*, mat4*> pair(&bone->bindOffset, &bone->inverseBindOffset);
			transformsArray.emplace_back(pair);
		}
	}
	*model = new Model({ meshes, skeleton, transformsArray });
}
