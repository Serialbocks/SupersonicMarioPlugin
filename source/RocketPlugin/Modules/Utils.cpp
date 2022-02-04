#include "Utils.h"

std::string Utils::GetBakkesmodFolderPath()
{
	wchar_t szPath[MAX_PATH];
	wchar_t* s = szPath;
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (LPWSTR)szPath)))
	{
		PathAppend((LPWSTR)szPath, _T("\\bakkesmod\\bakkesmod\\"));

		std::ostringstream stm;

		while (*s != L'\0') {
			stm << std::use_facet< std::ctype<wchar_t> >(std::locale()).narrow(*s++, '?');
		}
		return stm.str();
	}
	return "";
}