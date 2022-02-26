#pragma once

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"
#include "../Graphics/GraphicsTypes.h"

class Utils
{
public:
	std::string GetBakkesmodFolderPath();
	void ParseObjFile(std::string path, std::vector<Vertex> *outVertices);
	std::vector<std::string> SplitStr(std::string str, char delimiter);

};