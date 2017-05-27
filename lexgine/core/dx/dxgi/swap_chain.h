#ifndef LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H

#include <dxgi1_5.h>
#include <wrl.h>

#include <cstdint>

#include "../d3d12/command_queue.h"
#include "../../../osinteraction/windows/window.h"
#include "common.h"
#include "../../math/vector_types.h"
#include "../../multisampling.h"

using namespace Microsoft::WRL;


namespace lexgine {namespace core {namespace dx {namespace dxgi {


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
    friend class HwAdapter;    //!< only HwAdapters are having this vicious capability of creating swap chains
public:

    //! Retrieves Direct3D12 device associated with the swap chain
    dx::d3d12::Device& device();

    //! Retrieves default command queue associated with the swap chain
    d3d12::CommandQueue& defaultCommandQueue();

    //! Retrieves Window object to which the swap chain is attached
    osinteraction::windows::Window const& window() const;

    //! Retrieves current width and height of the swap chain packed into a 2D vector
    math::vector2u getDimensions() const;

private:
    SwapChain(ComPtr<IDXGIFactory4> const& dxgi_factory, d3d12::Device& device, osinteraction::windows::Window const& window, SwapChainDescriptor const& desc);

    ComPtr<IDXGIFactory4> m_dxgi_factory;   //!< DXGI factory used to create the swap chain
    d3d12::Device& m_device;    //!< Direct3D12 device interface
    osinteraction::windows::Window const& m_window;    //!< window, which holds the swap chain
    d3d12::CommandQueue m_default_command_queue;    //!< default command queue associated with the swap chain

    ComPtr<IDXGISwapChain3> m_dxgi_swap_chain;   //!< DXGI interface representing the swap chain

};


}}}}

#define LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H
#endif