#include "misc.h"
#include <fstream>
#include <sstream>

namespace lexgine{namespace core {namespace misc{

std::wstring asciiStringToWstring(std::string const& str)
{
    size_t len = str.size();
    wchar_t* wstr = new wchar_t[len];
    for (size_t i = 0; i < len; ++i) wstr[i] = static_cast<char>(str[i]);

    std::wstring rv{ wstr, len };
    delete[] wstr;

    return rv;
}

std::string wstringToAsciiString(std::wstring const& wstr)
{
    size_t len = wstr.size();
    char* str = new char[len];

    for (size_t i = 0; i < len; ++i)
        str[i] = static_cast<char>(wstr[i]);

    std::string rv{ str, len };

    delete[] str;

    return rv;
}

Optional<std::string> readAsciiTextFromSourceFile(std::string const& source_file)
{
    std::ifstream source_file_input_stream{ source_file.c_str(), std::ios_base::in };
    if (!source_file_input_stream)
        return Optional<std::string>{};

    std::ostringstream content_stream{ std::ios_base::out };
    content_stream << source_file_input_stream.rdbuf();
    source_file_input_stream.close();

    return Optional<std::string>{content_stream.str()};
}

bool doesFileExist(std::string const& file_path)
{
    return static_cast<bool>(std::ifstream(file_path.c_str(), std::ios::in));
}

}}}