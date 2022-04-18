#pragma once

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"
#include "../Graphics/GraphicsTypes.h"
#include <math.h>
#include <sys/stat.h>


#pragma comment(lib, "assimp-vc142-mt.lib")
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

class Utils
{
public:
	static std::wstring GetBakkesmodFolderPathWide()
	{
		wchar_t szPath[MAX_PATH];
		wchar_t* s = szPath;
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (LPWSTR)szPath)))
		{
			PathAppend((LPWSTR)szPath, _T("\\bakkesmod\\bakkesmod\\"));
			return szPath;
		}
		return L"";
	}

	static std::string GetBakkesmodFolderPath()
	{
		wchar_t* s = (wchar_t*)GetBakkesmodFolderPathWide().c_str();
		std::ostringstream stm;

		while (*s != L'\0') {
			stm << std::use_facet< std::ctype<wchar_t> >(std::locale()).narrow(*s++, '?');
		}
		return stm.str();
	}



	static void ProcessModelNode(aiNode* node, const aiScene* scene, std::vector<Vertex>* outVertices, std::vector<UINT>* outIndices)
	{
		for (UINT i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			for (UINT k = 0; k < mesh->mNumVertices; k++)
			{

			}
		}

		for (UINT i = 0; i < node->mNumChildren; i++)
		{
			ProcessModelNode(node->mChildren[i], scene, outVertices, outIndices);
		}
	}

	static bool LoadModel(std::string path, std::vector<Vertex>* outVertices, std::vector<UINT>* outIndices)
	{
		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
		if (scene == nullptr)
			return false;

		ProcessModelNode(scene->mRootNode, scene, outVertices, outIndices);

		return true;
	}

	static void ParseObjFile(std::string path, std::vector<Vertex>* vertices, std::vector<UINT>* indices)
	{
		std::ifstream file(path);
		std::string line;

		std::vector<Vertex> normals;
		while (std::getline(file, line))
		{
			if (line.size() == 0) continue;
			auto split = SplitStr(line, ' ');
			auto type = split[0];
			auto lastIndex = split.size() - 1;
			if (type == "v")
			{
				Vertex vertex;
				vertex.pos.x = std::stof(split[lastIndex - 2], nullptr);
				vertex.pos.y = std::stof(split[lastIndex - 1], nullptr);
				vertex.pos.z = std::stof(split[lastIndex], nullptr);
				vertex.color.x = 1.0f;
				vertex.color.y = 1.0f;
				vertex.color.z = 1.0f;
				vertex.color.w = 0.0f;
				vertex.texCoord.x = 1.0f;
				vertex.texCoord.y = 0.0f;
				vertices->push_back(vertex);
			}
			else if (type == "vn")
			{
				Vertex vertex;
				vertex.normal.x = std::stof(split[lastIndex - 2], nullptr);
				vertex.normal.y = std::stof(split[lastIndex - 1], nullptr);
				vertex.normal.z = std::stof(split[lastIndex], nullptr);
				normals.push_back(vertex);
			}
			else if (type == "f")
			{
				auto vertIndex1 = std::stoul(SplitStr(split[lastIndex - 2], '/')[0], nullptr);
				auto vertIndex2 = std::stoul(SplitStr(split[lastIndex - 1], '/')[0], nullptr);
				auto vertIndex3 = std::stoul(SplitStr(split[lastIndex], '/')[0], nullptr);
				auto normalIndex1 = std::stoul(SplitStr(split[lastIndex - 2], '/')[2], nullptr);
				auto normalIndex2 = std::stoul(SplitStr(split[lastIndex - 1], '/')[2], nullptr);
				auto normalIndex3 = std::stoul(SplitStr(split[lastIndex], '/')[2], nullptr);
				(*vertices)[vertIndex1].normal.x = normals[normalIndex1].normal.x;
				(*vertices)[vertIndex1].normal.y = normals[normalIndex1].normal.y;
				(*vertices)[vertIndex1].normal.z = normals[normalIndex1].normal.z;
				(*vertices)[vertIndex2].normal.x = normals[normalIndex2].normal.x;
				(*vertices)[vertIndex2].normal.y = normals[normalIndex2].normal.y;
				(*vertices)[vertIndex2].normal.z = normals[normalIndex2].normal.z;
				(*vertices)[vertIndex3].normal.x = normals[normalIndex3].normal.x;
				(*vertices)[vertIndex3].normal.y = normals[normalIndex3].normal.y;
				(*vertices)[vertIndex3].normal.z = normals[normalIndex3].normal.z;
				indices->push_back(vertIndex1);
				indices->push_back(vertIndex2);
				indices->push_back(vertIndex3);
			}
		}

	}



	static std::vector<std::string> SplitStr(std::string str, char delimiter)
	{
		std::stringstream stringStream(str);
		std::vector<std::string> seglist;
		std::string segment;
		while (std::getline(stringStream, segment, delimiter))
		{
			seglist.push_back(segment);
		}
		return seglist;
	}

	static float Distance(Vector v1, Vector v2)
	{
		return (float)sqrt(pow(v2.X - v1.X, 2.0) + pow(v2.Y - v1.Y, 2.0) + pow(v2.Z - v1.Z, 2.0));
	}

	static bool FileExists(std::string filename)
	{
		struct stat buffer;
		return (stat(filename.c_str(), &buffer) == 0);
	}

	static void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
		if (from.empty())
			return;
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	static inline void ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
	}

	// trim from end (in place)
	static inline void rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void trim(std::string& s) {
		ltrim(s);
		rtrim(s);
	}
};