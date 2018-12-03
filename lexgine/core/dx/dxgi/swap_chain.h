#ifndef LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H
#define LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H


#include <cstdint>

#include "common.h"
#include "lexgine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/math/vector_types.h"
#include "lexgine/core/multisampling.h"
#include "lexgine/osinteraction/windows/window.h"

using namespace Microsoft::WRL;


namespace lexgine {namespace core {namespace dx {namespace dxgi {

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
    ResourceUsage bufferUsage;
    uint32_t bufferCount;
    SwapChainScaling scaling;
    uint32_t refreshRate;
    bool windowed;
};


class SwapChain final : public NamedEntity<class_names::SwapChain>
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
    osinteraction::windows::Window const& window() const;

    //! Retrieves current width and height of the swap chain packed into a 2D vector
    math::Vector2u getDimensions() const;

    /*! Puts contents of the back buffer into the front buffer.
     When allow_frame_miss is 'true', it is allowed to skip the current frame if
     a newer frame is queued.
    */
    void present() const;

private:
    SwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory, 
        d3d12::Device& device, 
        d3d12::CommandQueue const& default_command_queue,
        osinteraction::windows::Window const& window, 
        SwapChainDescriptor const& desc);

private:
    ComPtr<IDXGIFactory6> m_dxgi_factory;   //!< DXGI factory used to create the swap chain
    d3d12::Device& m_device;    //!< Direct3D12 device interface
    osinteraction::windows::Window const& m_window;    //!< window, which holds the swap chain
    d3d12::CommandQueue const& m_default_command_queue;    //!< graphics command queue associated with the swap chain

    ComPtr<IDXGISwapChain4> m_dxgi_swap_chain;   //!< DXGI interface representing the swap chain

};


template<> class SwapChainAttorney<HwAdapter>
{
    friend class HwAdapter;

private:
    static SwapChain makeSwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory,
        d3d12::Device& device,
        d3d12::CommandQueue const& default_command_queue,
        osinteraction::windows::Window const& window,
        SwapChainDescriptor const& desc)
    {
        return SwapChain{ dxgi_factory, device, default_command_queue, window, desc };
    }
};



}}}}

#endif