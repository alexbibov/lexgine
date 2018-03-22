#ifndef LEXGINE_CORE_DX_DXCOMPILATION_COMMON_H
#define LEXGINE_CORE_DX_DXCOMPILATION_COMMON_H

#include <string>

namespace lexgine { namespace core { namespace dx { namespace dxcompilation {

enum class ShaderType
{
    vertex = 0, hull = 1, domain = 2, geometry = 3, pixel = 4, compute = 5,
    tessellation_control = 1, tesselation_evaluation = 2, fragment = 4    // OpenGL/Vulkan terminology for convenience
};

enum class ShaderModel : unsigned
{
    model_50 = 5,
    model_60 = 6,
    model_61 = (1 << 4) + 6,
    model_62 = (2 << 4) + 6
};

enum class HLSLCompilationOptimizationLevel : unsigned char
{
    level_no = static_cast<unsigned char>(-1),
    level0 = 0,
    level1,
    level2,
    level3,
    level4
};

//! Macro definition consumed by HLSL compiler
struct HLSLMacroDefinition
{
    std::string name;    //!< name of the macro to define
    std::string value;    //!< value of the macro to define
};

}}}}

#endif
