#pragma once

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"

class Utils
{
public:
	std::string GetBakkesmodFolderPath();
};