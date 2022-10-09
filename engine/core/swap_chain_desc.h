#ifndef LEXGINE_CORE_SWAP_CHAIN_DESC_H
#define LEXGINE_CORE_SWAP_CHAIN_DESC_H

#include <cstdint>
#include "engine/core/entity.h"
#include "engine/core/engine_api.h"
#include "engine/core/math/vector_types.h"
#include "engine/preprocessing/preprocessor_tokens.h"

namespace lexgine::core {

enum class LEXGINE_CPP_API SwapChainColorFormat
{
    r8_g8_b8_a8_unorm,
    b8_g8_r8_a8_unorm
};

enum class LEXGINE_CPP_API SwapChainDepthFormat
{
    depth16,
    depth32,
    depth24_stencil8,
    depth32_stencil8,
};

struct LEXGINE_CPP_API DEPENDS_ON(SwapChainColorFormat) SwapChainDescriptor
{
    SwapChainColorFormat color_format;
    uint32_t back_buffer_count;
    bool enable_vsync;
    bool windowed;
};


}

#endif