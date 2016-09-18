#include "misc.h"

std::wstring lexgine::core::misc::ascii_string_to_wstring(std::string const& str)
{
    size_t len = str.size();
    wchar_t* wstr = new wchar_t[len];
    for (size_t i = 0; i < len; ++i) wstr[i] = static_cast<char>(str[i]);

    std::wstring rv{ wstr[0], wstr[len - 1] };
    delete[] wstr;

    return rv;
}