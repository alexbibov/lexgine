#ifndef LEXGINE_CORE_SHADER_SOURCE_CODE_PREPROCESSOR_H

#include <string>

#include "entity.h"
#include "class_names.h"

namespace lexgine {namespace core {

/*! Implements preprocessing of shader source code, which does not depend on the shading language
 This API is OS-agnostic
*/
class ShaderSourceCodePreprocessor : public NamedEntity<class_names::ShaderSourceCodePreprocessor>
{
public:

    //! Determines the type of provided source code
    enum class SourceType{file, string};

    /*! Performs preprocessing of provided source code.
     The type of the source depends on the value of parameter source_type. If source_type=file then
     the source is treated as path to a file containing the shading code. If source_type=string then
     parameter source is assumed to contain raw shading source code
    */
    ShaderSourceCodePreprocessor(std::string const& source, 
        SourceType source_type, 
        const std::vector<std::string>& lookup_directories);

    ~ShaderSourceCodePreprocessor();

    char const* getPreprocessedSource() const;    //! returns string containing preprocessed shader source code


private:
    std::string m_path;    //!< contains path to the shader when applicable
    std::string m_shader_source;    //!< contains original shader source code
    std::string m_preprocessed_shader_source;    //!< contains preprocessed shader source code


private:

    class impl;
    std::unique_ptr<impl> m_impl;    //!< conceals implementation details
};

}}


#define LEXGINE_CORE_SHADER_SOURCE_CODE_PREPROCESSOR_H
#endif
