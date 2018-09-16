#include "swap_chain.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/dx/d3d12/command_queue.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::dxgi;


Device& lexgine::core::dx::dxgi::SwapChain::device() const
{
    return m_device;
}

CommandQueue const& SwapChain::defaultCommandQueue() const
{
    return m_default_command_queue;
}

osinteraction::windows::Window const& SwapChain::window() const
{
    return m_window;
}

math::vector2u SwapChain::getDimensions() const
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
    m_dxgi_swap_chain->GetDesc1(&swap_chain_desc1);

    return math::vector2u{ swap_chain_desc1.Width, swap_chain_desc1.Height };
}

SwapChain::SwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory,
    Device& device,
    CommandQueue const& default_command_queue,
    osinteraction::windows::Window const& window,
    SwapChainDescriptor const& desc):
    m_dxgi_factory{ dxgi_factory },
    m_device{ device },
    m_window{ window },
    m_default_command_queue{ default_command_queue }
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1{};
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fs_desc{};

    math::vector2u output_window_dimensions = m_window.getDimensions();
    swap_chain_desc1.Width = output_window_dimensions.x;
    swap_chain_desc1.Height = output_window_dimensions.y;
    swap_chain_desc1.Format = desc.format;
    swap_chain_desc1.Stereo = desc.stereo;
    swap_chain_desc1.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
    swap_chain_desc1.BufferUsage = static_cast<DXGI_USAGE>(desc.bufferUsage);
    swap_chain_desc1.BufferCount = desc.bufferCount;
    swap_chain_desc1.Scaling = static_cast<DXGI_SCALING>(desc.scaling);
    swap_chain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    swap_chain_fs_desc.RefreshRate = DXGI_RATIONAL{ desc.refreshRate, 1 };
    swap_chain_fs_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_fs_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_chain_fs_desc.Windowed = desc.windowed;

    IDXGISwapChain1* p_swap_chain1 = nullptr;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_dxgi_factory->CreateSwapChainForHwnd(m_default_command_queue.native().Get(), 
            m_window.native(), &swap_chain_desc1, &swap_chain_fs_desc, NULL, &p_swap_chain1),
        S_OK
    );

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        p_swap_chain1->QueryInterface(IID_PPV_ARGS(&m_dxgi_swap_chain)),
        S_OK
    );
    p_swap_chain1->Release();
}
