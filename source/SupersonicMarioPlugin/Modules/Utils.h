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

	template<typename ... Args>
	static std::string StringFormat(const std::string& format, Args ... args)
	{
		int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
		auto size = static_cast<size_t>(size_s);
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
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

	static inline uint8_t* readFileAlloc(std::string path, size_t* fileLength)
	{
		FILE* f;
		fopen_s(&f, path.c_str(), "rb");

		if (!f) return NULL;

		fseek(f, 0, SEEK_END);
		size_t length = (size_t)ftell(f);
		rewind(f);
		uint8_t* buffer = (uint8_t*)malloc(length + 1);
		if (buffer != NULL)
		{
			fread(buffer, 1, length, f);
			buffer[length] = 0;
		}

		fclose(f);

		if (fileLength) *fileLength = length;

		return (uint8_t*)buffer;
	}
};