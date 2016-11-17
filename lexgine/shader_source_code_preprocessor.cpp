#include "shader_source_code_preprocessor.h"
#include "exception.h"

#include <fstream>

using namespace lexgine::core;


namespace
{

std::string readSourceFile(std::string const& source_file)
{
    std::ifstream ifile{ source_file };
    if (!ifile)
        throw lexgine::core::Exception{ "Unable to locate shader source file \"" + source_file + "\"" };

    ifile.seekg(0, std::ios_base::end);
    std::streampos file_end_pos = ifile.tellg();
    ifile.seekg(0, std::ios_base::beg);
    std::streampos file_start_pos = ifile.tellg();

    std::size_t file_length = static_cast<size_t>(file_end_pos - file_start_pos);
    char *read_buffer = new char[file_length + 1];
    ifile.get(read_buffer, static_cast<std::streamsize>(file_length));

    std::string rv{ read_buffer };
    delete[] read_buffer;

    return rv;
}


std::string resolveIncludeDirectivesInShaderSource(std::string const& shader_source_code)
{


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


}
