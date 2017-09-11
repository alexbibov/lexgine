#include "misc.h"
#include <fstream>
#include <sstream>
#include <limits>

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

unsigned long long getFileSize(std::string const& file_path)
{
    if (!doesFileExist(file_path)) return 0;

    std::filebuf fbuf{};
    if (!fbuf.open(file_path, std::ios_base::in)) return 0;

    std::streampos file_start_pos = fbuf.pubseekoff(0, std::ios_base::beg);
    std::streampos file_end_pos = fbuf.pubseekoff(0, std::ios_base::end);

    std::streamoff file_size = file_end_pos - file_start_pos;

    fbuf.close();
    return static_cast<unsigned long long>(file_size);
}

void readBinaryDataFromSourceFile(std::string const& file_path, void* destination_memory_address)
{
    std::ifstream ifile{ file_path, std::ios_base::in | std::ios_base::binary };

    if (ifile)
    {
        std::filebuf* p_read_buffer = ifile.rdbuf();
        p_read_buffer->sgetn(static_cast<char*>(destination_memory_address), (std::numeric_limits<std::streamsize>::max)());
    }

    ifile.close();
}

bool writeBinaryDataToFile(std::string const& file_path, void* source_memory_address, size_t data_size)
{
    std::ofstream ofile{ file_path, std::ios_base::out | std::ios_base::binary };
    if (!ofile) return false;

    std::filebuf* p_write_buffer = ofile.rdbuf();
    if (p_write_buffer->sputn(static_cast<char*>(source_memory_address), static_cast<std::streamsize>(data_size)) != data_size)
        return false;
    return true;
}

}}}