#ifndef LEXGINE_CORE_DX_DXGI_COMMON_H

#include <dxgi1_5.h>
#include "flags.h"

namespace lexgine {namespace core {namespace dx {namespace dxgi {


namespace __tag {
enum class tagDXGIUsage : DXGI_USAGE
{
    back_buffer = DXGI_USAGE_BACK_BUFFER,
    read_only = DXGI_USAGE_READ_ONLY,
    render_target = DXGI_USAGE_RENDER_TARGET_OUTPUT,
    shader_input = DXGI_USAGE_SHADER_INPUT,
    shared = DXGI_USAGE_SHARED,
    unordered_access = DXGI_USAGE_UNORDERED_ACCESS
};
}


//! Describes flags that describe resource usage scenarios. Some flags cannot be combined (see documentation for details)
using ResourceUsage = misc::Flags<__tag::tagDXGIUsage, DXGI_USAGE>;


}}}}

#define LEXGINE_CORE_DX_DXGI_COMMON_H
#endif
