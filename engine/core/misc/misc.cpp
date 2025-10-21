#include <fstream>
#include <sstream>
#include <limits>
#include <cstdio>
#include <algorithm>

#include <windows.h>

#include "misc.h"


namespace lexgine::core::misc {

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

bool readBinaryDataFromSourceFile(std::string const& file_path, void* destination_memory_address)
{
    std::ifstream ifile{ file_path, std::ios_base::in | std::ios_base::binary };

    if (ifile)
    {
        std::filebuf* p_read_buffer = ifile.rdbuf();
        p_read_buffer->sgetn(static_cast<char*>(destination_memory_address), (std::numeric_limits<std::streamsize>::max)());

        ifile.close();

        return true;
    }

    return false;
}

Optional<std::vector<uint8_t>> readBinaryDataFromSourceFile(std::string const& file_path)
{
    std::ifstream ifile{ file_path, std::ios_base::in | std::ios_base::binary };

    if (ifile)
    {
        std::vector<uint8_t> rv{};
        std::transform(std::istreambuf_iterator<char>{ifile}, std::istreambuf_iterator<char>{},
            std::back_inserter(rv), [](char c) {return static_cast<uint8_t>(c); });

        ifile.close();
        return Optional<std::vector<uint8_t>>{rv};
    }

    return Optional<std::vector<uint8_t>>{};
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


Optional<DateTime> getFileLastUpdatedTimeStamp(std::string const& file_path)
{
    HANDLE hfile = CreateFile(asciiStringToWstring(file_path).c_str(),
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) return Optional<DateTime>{};

    FILETIME last_updated_time{};
    if (!GetFileTime(hfile, NULL, NULL, &last_updated_time)) return Optional<DateTime>{};

    // Convert file time to UTC time
    SYSTEMTIME systime{};
    if (!FileTimeToSystemTime(&last_updated_time, &systime)) return Optional<DateTime>{};

    return DateTime{ systime.wYear, static_cast<misc::Month>(systime.wMonth), static_cast<uint8_t>(systime.wDay),
        static_cast<uint8_t>(systime.wHour), static_cast<uint8_t>(systime.wMinute),
        systime.wSecond + systime.wMilliseconds / 1000.0 };
}

std::list<std::string> getFilesInDirectory(std::string const& directory_name, std::string const& name_pattern)
{
    {
        DWORD file_attributes = GetFileAttributes(asciiStringToWstring(directory_name).c_str());
        if (file_attributes == INVALID_FILE_ATTRIBUTES
            || !(file_attributes & FILE_ATTRIBUTE_DIRECTORY))
            return std::list<std::string>{};
    }

    WIN32_FIND_DATA data{};
    HANDLE search_handle = FindFirstFile(asciiStringToWstring(directory_name + name_pattern).c_str(), &data);
    if (search_handle == INVALID_HANDLE_VALUE)
        return std::list<std::string>{};

    std::list<std::string> rv{};
    if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        rv.push_back(wstringToAsciiString(data.cFileName));
    }


    while (FindNextFile(search_handle, &data))
    {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            rv.push_back(wstringToAsciiString(data.cFileName));
        }
    }

    FindClose(search_handle);

    return rv;
}

uint32_t getSetBitCount(uint64_t value)
{
    uint64_t rv{ 0U };

#if defined(_WIN64) && defined(_M_AMD64)
    return static_cast<uint32_t>(__popcnt64(value));
#else
    rv = value - ((value >> 1) & 0x5555555555555555);
    rv = (rv & 0x3333333333333333) + ((rv >> 2) & 0x3333333333333333);
    rv = (rv & 0x0F0F0F0F0F0F0F0F) + ((rv >> 4) & 0x0F0F0F0F0F0F0F0F);
    rv = (rv + (rv >> 8)) & 0x00FF00FF00FF00FF;
    rv = (rv + (rv >> 16)) & 0x0000FFFF0000FFFF;
    rv = (rv & 0x00000000FFFFFFFF) + (rv >> 32);
    return static_cast<uint32_t>(rv);
#endif
}

std::string formatString(char const* format_string, ...)
{
    std::unique_ptr<char[]> tmp;

    int processed_length, allocated_length = static_cast<int>(strlen(format_string));

    va_list va;
    do
    {
        allocated_length *= 2;
        tmp.reset(new char[allocated_length]);
        va_start(va, format_string);
        processed_length = vsnprintf(tmp.get(), allocated_length, format_string, va);
        va_end(va);

        if (processed_length < 0) throw;

    } while (processed_length > allocated_length - 1);

    return std::string{ tmp.get() };
}


std::string toLowerCase(std::string const& str)
{
    std::vector<char> aux{};
    std::transform(str.begin(), str.end(), std::back_inserter(aux),
        [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return std::string{ aux.begin(), aux.end() };
}

}