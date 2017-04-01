#include "misc.h"

namespace lexgine{namespace core {namespace misc{

std::wstring ascii_string_to_wstring(std::string const& str)
{
    size_t len = str.size();
    wchar_t* wstr = new wchar_t[len];
    for (size_t i = 0; i < len; ++i) wstr[i] = static_cast<char>(str[i]);

    std::wstring rv{ wstr, len };
    delete[] wstr;

    return rv;
}

std::string wstring_to_ascii_string(std::wstring const& wstr)
{
    size_t len = wstr.size();
    char* str = new char[len];

    for (size_t i = 0; i < len; ++i)
        str[i] = static_cast<char>(wstr[i]);

    std::string rv{ str, len };

    delete[] str;

    return rv;
}

}}}