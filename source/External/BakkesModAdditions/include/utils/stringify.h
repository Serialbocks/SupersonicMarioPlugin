#pragma once
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>


template<typename T>
std::string to_hex(const T number)
{
    std::stringstream ss;
    std::make_unsigned_t<T> unsigned_number = number;
    ss << std::setw(sizeof number * 2) << std::setfill('0') << std::hex << static_cast<size_t>(unsigned_number);
    
    return ss.str();
}


template<typename T>
std::string to_small_hex(const T number)
{
    std::stringstream ss;
    std::make_unsigned_t<T> unsigned_number = number;
    ss << std::hex << static_cast<size_t>(unsigned_number);

    return ss.str();
}


template<typename T>
std::string to_hex(const T* buffer, const size_t size)
{
    std::string str = "[";
    for (size_t i = 0; i < size; i++) {
        str += " " + to_hex(buffer[i]);
        if (i + 1 < size) {
            str += ",";
        }
    }

    return str + " ]";
}


inline std::string get_addr(const void* ptr)
{
    std::stringstream ss;
    ss << ptr;

    return "0x" + ss.str();
}


inline std::string to_lower(const std::string& str)
{
    std::string str_cpy = str;
    std::ranges::transform(str_cpy, str_cpy.begin(),
        [](const unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });

    return str_cpy;
}

inline std::wstring to_lower(const std::wstring& wstr)
{
    std::wstring wstr_cpy = wstr;
    std::ranges::transform(wstr_cpy, wstr_cpy.begin(),
        [](const wchar_t c) { return static_cast<wchar_t>(std::tolower(c)); });

    return wstr_cpy;
}


inline std::string to_upper(const std::string& str)
{
    std::string str_cpy = str;
    std::ranges::transform(str_cpy, str_cpy.begin(),
        [](const unsigned char c) { return static_cast<unsigned char>(std::toupper(c)); });

    return str_cpy;
}

inline std::wstring to_upper(const std::wstring& wstr)
{
    std::wstring wstr_cpy = wstr;
    std::ranges::transform(wstr_cpy, wstr_cpy.begin(),
        [](const wchar_t c) { return static_cast<wchar_t>(std::toupper(c)); });

    return wstr_cpy;
}


inline std::string quote(const std::string& str)
{
    std::stringstream ss;
    ss << std::quoted(str);
    return ss.str();
}

inline std::wstring quote(const std::wstring& wstr)
{
    std::wstringstream wss;
    wss << std::quoted(wstr);
    return wss.str();
}


inline std::string concat(const std::string& str1, const std::string& str2)
{
    return str1 + str2;
}

inline std::wstring concat(const std::wstring& wstr1, const std::wstring& wstr2)
{
    return wstr1 + wstr2;
}


inline std::string to_string(const std::string& str)
{
    return str;
}

inline std::string to_string(const std::wstring& wstr)
{
    const std::filesystem::path p(wstr);

    return p.string();
}


inline std::wstring to_wstring(const std::wstring& wstr)
{
    return wstr;
}

inline std::wstring to_wstring(const std::string& str)
{
    const std::filesystem::path p(str);

    return p.wstring();
}


struct case_insensitive_less {
    bool operator()(const std::string& left, const std::string& right) const {
        return to_lower(left) < to_lower(right);
    }

    bool operator()(const std::wstring& left, const std::wstring& right) const {
        return to_lower(left) < to_lower(right);
    }
};
