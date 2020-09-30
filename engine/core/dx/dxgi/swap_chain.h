#ifndef LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H
#define LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H


#include <cstdint>
#include <dxgi1_6.h>
#include <wrl.h>

#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/multisampling.h"
#include "engine/osinteraction/windows/window.h"


using namespace Microsoft::WRL;


namespace lexgine::core::dx::dxgi {

template<typename T> class SwapChainAttorney;

//! Describes swap chain scaling modes
enum class SwapChainScaling
{
    stretch = DXGI_SCALING_STRETCH,
    none = DXGI_SCALING_NONE,
    stretch_and_keep_aspect_ratio = DXGI_SCALING_ASPECT_RATIO_STRETCH
};


//! Describes parameters of the swap chain
struct SwapChainDescriptor
{
    DXGI_FORMAT format;    //!< display format
    bool stereo;
    SwapChainScaling scaling;
    uint32_t refreshRate;
    bool windowed;
    bool enable_vsync;
    uint32_t back_buffer_count;
};

class SwapChain final : public NamedEntity<class_names::DXGI_SwapChain>
{
    friend class SwapChainAttorney<HwAdapter>;

public:
    SwapChain(SwapChain const&) = delete;
    SwapChain(SwapChain&&) = default;

public:

    //! Retrieves Direct3D12 device associated with the swap chain
    dx::d3d12::Device& device() const;

    //! Retrieves graphics command queue associated with the swap chain
    d3d12::CommandQueue const& defaultCommandQueue() const;

    //! Retrieves Window object to which the swap chain is attached
    osinteraction::windows::Window& window() const;

    //! Retrieves current width and height of the swap chain packed into a 2D vector
    math::Vector2u getDimensions() const;

    //! Retrieves one of the back buffers of the swap chain and wrappes it into a Resource object
    dx::d3d12::Resource getBackBuffer(uint32_t buffer_index) const;

    //! Returns index of the current back buffer of the swap chain
    uint32_t getCurrentBackBufferIndex() const;

    //! Puts contents of the back buffer into the front buffer.
    void present() const;

    //! Total back buffer count
    uint32_t backBufferCount() const;

    //! Retrieves descriptor of the swap chain
    SwapChainDescriptor const& descriptor() const;

    //! Resizes the swap chain 
    void resizeBuffers(math::Vector2u const& new_dimensions);

    //! Checks if the swap chain is in idle state
    bool isIdle() const { return m_swapChainIsIdle; }

private:
    SwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory, 
        d3d12::Device& device, 
        d3d12::CommandQueue const& default_command_queue,
        osinteraction::windows::Window& window, 
        SwapChainDescriptor const& desc);

private:
    ComPtr<IDXGIFactory6> m_dxgi_factory;   //!< DXGI factory used to create the swap chain
    d3d12::Device& m_device;    //!< Direct3D12 device interface
    osinteraction::windows::Window& m_window;    //!< window, which holds the swap chain
    d3d12::CommandQueue const& m_default_command_queue;    //!< graphics command queue associated with the swap chain
    SwapChainDescriptor m_descriptor;    //!< Descriptor of the swap chain
    ComPtr<IDXGISwapChain4> m_dxgi_swap_chain;   //!< DXGI interface representing the swap chain
    mutable bool m_swapChainIsIdle{ false };    //!< determines if swap chain is in idle state
};


template<> class SwapChainAttorney<HwAdapter>
{
    friend class HwAdapter;

private:
    static SwapChain makeSwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory,
        d3d12::Device& device,
        d3d12::CommandQueue const& default_command_queue,
        osinteraction::windows::Window& window,
        SwapChainDescriptor const& desc)
    {
        return SwapChain{ dxgi_factory, device, default_command_queue, window, desc };
    }
};



}

#endif