#include "shader_source_code_preprocessor.h"
#include "exception.h"

#include <fstream>
#include <regex>

using namespace lexgine::core;


namespace
{

std::string readSourceFile(std::string const& source_file)
{
    std::ifstream ifile{ source_file };
    if (!ifile)
        throw lexgine::core::Exception{ "Unable to open shader source file \"" + source_file + "\"" };

    ifile.seekg(0, std::ios_base::end);
    std::streampos file_end_pos = ifile.tellg();
    ifile.seekg(0, std::ios_base::beg);
    std::streampos file_start_pos = ifile.tellg();

    std::size_t file_length = static_cast<size_t>(file_end_pos - file_start_pos);
    char *read_buffer = new char[file_length + 1];
    ifile.get(read_buffer, static_cast<std::streamsize>(file_length));

    std::string rv{ read_buffer };
    delete[] read_buffer;

    ifile.close();

    return rv;
}


inline size_t get_current_line_end_index(std::string const& str, size_t search_start_index)
{
    size_t rv = search_start_index;
    while (str[rv] != '\n') ++rv;
    return rv;
}


std::string resolveIncludeDirectivesInShaderSource(std::string const& shader_source_code)
{
    std::regex const include_directive_syntax{ R"%(^\s*//!include\s*(".")\s*$)%" };
    std::string processed_source_code{ shader_source_code };

    size_t line_start = 0U, line_end = 0U;
    while (line_start < processed_source_code.length())
    {
        line_end = get_current_line_end_index(processed_source_code, line_start);
        std::string line{ processed_source_code.begin() + line_start, processed_source_code.begin() + line_end + 1 };
        std::smatch m;
        if (!std::regex_match(line, m, include_directive_syntax))
        {
            line_start = line_end + 1;
        }
        else
        {
            std::string include_path{ m[1].str().begin() + 1 , m[1].str().end() - 1 };
            std::string included_source = resolveIncludeDirectivesInShaderSource(readSourceFile(include_path));

            processed_source_code = processed_source_code.substr(0, line_start) + included_source + processed_source_code.substr(line_end + 1);

            line_start += included_source.length();
        }
    }

    return processed_source_code;
}


}


ShaderSourceCodePreprocessor::ShaderSourceCodePreprocessor(std::string const& source, SourceType source_type)
{
    switch (source_type)
    {
    case ShaderSourceCodePreprocessor::SourceType::file:
        m_shader_source = readSourceFile(source);
        break;

    case ShaderSourceCodePreprocessor::SourceType::string:
        m_shader_source = source;
        break;
    }

    m_preprocessed_shader_source = resolveIncludeDirectivesInShaderSource(m_shader_source);
}

char const* ShaderSourceCodePreprocessor::getPreprocessedSource() const
{
    return m_preprocessed_shader_source.c_str();
}
