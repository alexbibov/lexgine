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

osinteraction::windows::Window& SwapChain::window() const
{
    return m_window;
}

math::Vector2u SwapChain::getDimensions() const
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
    LEXGINE_THROW_ERROR_IF_FAILED(this,
        m_dxgi_swap_chain->GetDesc1(&swap_chain_desc1),
        S_OK);

    return math::Vector2u{ swap_chain_desc1.Width, swap_chain_desc1.Height };
}

dx::d3d12::Resource SwapChain::getBackBuffer(uint32_t buffer_index) const
{
    ComPtr<ID3D12Resource> backbuffer_interface;

    LEXGINE_THROW_ERROR_IF_FAILED(this,
        m_dxgi_swap_chain->GetBuffer(static_cast<UINT>(buffer_index), IID_PPV_ARGS(backbuffer_interface.GetAddressOf())),
        S_OK);

    return dx::d3d12::Resource{ backbuffer_interface };
}

uint32_t SwapChain::getCurrentBackBufferIndex() const
{
    return static_cast<uint32_t>(m_dxgi_swap_chain->GetCurrentBackBufferIndex());
}

void SwapChain::present() const
{
    LEXGINE_THROW_ERROR_IF_FAILED(this, m_dxgi_swap_chain->Present(1, 0), S_OK);
}

uint32_t SwapChain::backBufferCount() const
{
    return m_advanced_parameters.queued_buffer_count;
}

SwapChainDescriptor const& SwapChain::descriptor() const
{
    return m_descriptor;
}

void SwapChain::resizeBuffers(math::Vector2u const& new_dimensions)
{
    // TODO: AFR support
    
    std::vector<UINT> node_masks(m_advanced_parameters.queued_buffer_count, 0x1);
    std::vector<IUnknown*> queues(m_advanced_parameters.queued_buffer_count, m_default_command_queue.native().Get());

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_dxgi_swap_chain->ResizeBuffers1(0,
            static_cast<UINT>(new_dimensions.x), static_cast<UINT>(new_dimensions.y),
            DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH, node_masks.data(), queues.data()),
        S_OK
    );
}

SwapChain::SwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory,
    Device& device,
    CommandQueue const& default_command_queue,
    osinteraction::windows::Window& window,
    SwapChainDescriptor const& desc, SwapChainAdvancedParameters const& advanced_parameters)
    : m_dxgi_factory{ dxgi_factory }
    , m_device{ device }
    , m_window{ window }
    , m_default_command_queue{ default_command_queue }
    , m_descriptor{ desc }
    , m_advanced_parameters{ advanced_parameters }
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1{};
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fs_desc{};

    math::Vector2f client_area_size = m_window.getClientArea().size();
    swap_chain_desc1.Width = static_cast<UINT>(client_area_size.x);
    swap_chain_desc1.Height = static_cast<UINT>(client_area_size.y);
    swap_chain_desc1.Format = desc.format;
    swap_chain_desc1.Stereo = desc.stereo;
    swap_chain_desc1.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
    swap_chain_desc1.BufferUsage = advanced_parameters.back_buffer_usage_scenario;
    swap_chain_desc1.BufferCount = advanced_parameters.queued_buffer_count;
    swap_chain_desc1.Scaling = static_cast<DXGI_SCALING>(desc.scaling);
    swap_chain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
