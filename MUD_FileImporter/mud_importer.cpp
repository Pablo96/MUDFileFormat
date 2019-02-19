#include "tinyxml2.h"
#include "mud_importer.hpp"
#include <sstream>	//stringstream
#include <iostream> // cout
#include <algorithm> // sort

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

void helperStrToQuat(const char* charArray, MUDLoader::quat& vec)
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





MUDLoader::mat4 helperBuildInverseBindAbsolute(MUDLoader::Bone* bone)
{
	using namespace MUDLoader;

	int i = 0;
	mat4 transform = bone->bindOffset;

	for (Bone* parent = bone->parent; parent != nullptr; parent = parent->parent)
	{
		transform = parent->bindOffset * transform;
		i++;
	}

	return inverseMat4(transform);
}

void helperBoneBuild(MUDLoader::Bone& bone, tinyxml2::XMLElement* boneNode)
{
	using namespace MUDLoader;

	auto translation = boneNode->FindAttribute("translation")->Value();
	auto rotation = boneNode->FindAttribute("rotation")->Value();
	auto name = boneNode->FindAttribute("name")->Value();
	auto id = boneNode->FindAttribute("id")->Value();
	bone.debugName = name;
	bone.id = atoi(id);

	vec3 translationVec = vec3();
	helperStrToVec3(translation, translationVec);

	quat rotationQuat;
	helperStrToQuat(rotation, rotationQuat);

	quaternionToMatrix(rotationQuat, bone.bindOffset);
	translationToMatrix(translationVec, bone.bindOffset);

	bone.inverseBindOffset = helperBuildInverseBindAbsolute(&bone);
}


void skeletonBuild(tinyxml2::XMLElement* node, MUDLoader::Bone* parent, std::vector<MUDLoader::Bone*>& array)
{
	for (auto siblingNode = node; siblingNode != nullptr; siblingNode = siblingNode->NextSiblingElement("bone"))
	{
		MUDLoader::Bone* boneTmp = new MUDLoader::Bone();
		
		// double linking
		parent->children.emplace_back(boneTmp);
		boneTmp->parent = parent;
		
		// bone data
		helperBoneBuild(*boneTmp, siblingNode);
		
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
	std::vector<tuple<int, mat4*, mat4*>> transformsArray;

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
			int id;
			if (bone->parent)
				id = bone->parent->id;
			else
				id = -1;
			tuple<int, mat4*, mat4*> pair = { id, &bone->bindOffset, &bone->inverseBindOffset };
			transformsArray.emplace_back(pair);
		}
	}
	*model = new Model({ meshes, skeleton, transformsArray });
}


void MUDLoader::quaternionToMatrix(quat& q, mat4& mat)
{
	mat = glm::toMat4(q);
}

void MUDLoader::translationToMatrix(vec3& vec, mat4& mat)
{
	mat4 location;
	glm::translate(location, vec);
	mat = location * mat;
}

MUDLoader::mat4 MUDLoader::inverseMat4(const mat4& matrix)
{
	return glm::inverse(matrix);
}