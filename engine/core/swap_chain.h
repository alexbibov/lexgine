#ifndef LEXGINE_CORE_SWAP_CHAIN_H
#define LEXGINE_CORE_SWAP_CHAIN_H

#include <cstdint>
#include "engine/core/entity.h"
#include "engine/core/engine_api.h"
#include "engine/osinteraction/window_handler.h"
#include "engine/preprocessing/preprocessor_tokens.h"

namespace lexgine::core {

enum class LEXGINE_CPP_API SwapChainColorFormat
{
    r8_g8_b8_a8_unorm,
    b8_g8_r8_a8_unorm,
    r8_g8_b8_a8_unorm_srgb,
    b8_g8_r8_a8_unorm_srgb
};

struct LEXGINE_CPP_API DEPENDS_ON(SwapChainColorFormat) SwapChainDescriptor
{
    SwapChainColorFormat color_format;
    uint32_t back_buffer_count;
    bool enable_vsync;
    bool windowed;
};

class LEXGINE_CPP_API DEPENDS_ON(SwapChainDescriptor) SwapChain : public NamedEntity<class_names::SwapChain>, public EngineApiAwareObject
{
public:
    virtual ~SwapChain() = default;

    //! Retrieves current width and height of the swap chain packed into a 2D vector
    LEXGINE_CPP_API virtual math::Vector2u getDimensions() const = 0;

    //! Returns index of the current back buffer of the swap chain
    LEXGINE_CPP_API virtual uint32_t getCurrentBackBufferIndex() const = 0;

    //! Puts contents of the back buffer into the front buffer.
    LEXGINE_CPP_API virtual void present() const = 0;

    //! Total back buffer count
    LEXGINE_CPP_API virtual uint32_t backBufferCount() const = 0;

    //! Resizes the swap chain 
    LEXGINE_CPP_API virtual void resizeBuffers(math::Vector2u const& new_dimensions) = 0;

    //! Checks if the swap chain is in idle state
    LEXGINE_CPP_API virtual bool isIdle() const = 0;

    //! Retrieves swap chain descriptor
    LEXGINE_CPP_API virtual SwapChainDescriptor descriptor() const = 0;

protected:
    SwapChain(EngineApi engine_api);
};



}

#endif