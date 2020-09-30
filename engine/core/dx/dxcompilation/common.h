#ifndef LEXGINE_CORE_DX_DXCOMPILATION_COMMON_H
#define LEXGINE_CORE_DX_DXCOMPILATION_COMMON_H

#include <string>

namespace lexgine { namespace core { namespace dx { namespace dxcompilation {

enum class ShaderType
{
    vertex = 0, hull = 1, domain = 2, geometry = 3, pixel = 4, compute = 5,
    tessellation_control = 1, tesselation_evaluation = 2, fragment = 4    // OpenGL/Vulkan terminology for convenience
};

enum class ShaderModel : unsigned short
{
    model_50 = 5 << 4,
    model_60 = 6 << 4,
    model_61 = (6 << 4) | 1,
    model_62 = (6 << 4) | 2
};

enum class HLSLCompilationOptimizationLevel : unsigned char
{
    level_no = static_cast<unsigned char>(-1),
    level0 = 0,
    level1,
    level2,
    level3
};

//! Macro definition consumed by HLSL compiler
struct HLSLMacroDefinition
{
    std::string name;    //!< name of the macro to define
    std::string value;    //!< value of the macro to define
};

}}}}

#endif
