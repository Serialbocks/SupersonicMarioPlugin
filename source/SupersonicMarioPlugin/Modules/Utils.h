#pragma once

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"
#include "../Graphics/GraphicsTypes.h"
#include <math.h>
#include <sys/stat.h>
#include <filesystem>
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

	static std::string GetMapFolderPath()
	{
		char filePath[MAX_PATH];
		GetModuleFileNameA(NULL, filePath, MAX_PATH);
		std::filesystem::path exePath(filePath);
		
		return exePath
			.parent_path()
			.parent_path()
			.parent_path()
			.append("TAGame\\CookedPCConsole")
			.string();
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