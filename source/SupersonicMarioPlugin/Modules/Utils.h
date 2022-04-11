#pragma once

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"
#include "../Graphics/GraphicsTypes.h"
#include <math.h>
#include <sys/stat.h>

class Utils
{
public:
	std::wstring GetBakkesmodFolderPathWide();
	std::string GetBakkesmodFolderPath();
	void ParseObjFile(std::string path, std::vector<Vertex> *outVertices);
	std::vector<std::string> SplitStr(std::string str, char delimiter);
	float Distance(Vector v1, Vector v2);
	bool FileExists(std::string filename);
	void ReplaceAll(std::string& str, const std::string& from, const std::string& to);

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