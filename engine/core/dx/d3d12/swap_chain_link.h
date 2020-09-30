#ifndef LEXGINE_CORE_DX_D3D12_SWAP_CHAIN_LINK_H
#define LEXGINE_CORE_DX_D3D12_SWAP_CHAIN_LINK_H

#include <vector>
#include <thread>
#include <functional>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"

#include "engine/osinteraction/listener.h"
#include "engine/osinteraction/windows/window_listeners.h"

#include "resource.h"
#include "rendering_target.h"


namespace lexgine::core::dx::d3d12 {

enum class SwapChainDepthBufferFormat
{
    d32float_s8x24_uint = DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    d32float = DXGI_FORMAT_D32_FLOAT,
    d24unorm_s8uint = DXGI_FORMAT_D24_UNORM_S8_UINT,
    d16unorm = DXGI_FORMAT_D16_UNORM
};

//! Implements link between swap chain and main rendering loop
class SwapChainLink final : 
    public osinteraction::Listeners<osinteraction::windows::WindowSizeChangeListener>
{
public:
    static std::shared_ptr<SwapChainLink> create(Globals& globals, dxgi::SwapChain& swap_chain_to_link,
        SwapChainDepthBufferFormat depth_buffer_format);

public:
    ~SwapChainLink();

    SwapChainLink(SwapChainLink const&) = delete;
    SwapChainLink(SwapChainLink&& other);

    void linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link);

    void render();

public:    // WindowSizeChangeListener events
    bool minimized() override;
    bool maximized(uint16_t new_width, uint16_t new_height) override;
    bool size_changed(uint16_t new_width, uint16_t new_height) override;

private:
    SwapChainLink(Globals& globals, dxgi::SwapChain& swap_chain_to_link,
        SwapChainDepthBufferFormat depth_buffer_format);


    void updateRenderingConfiguration() const;
    void releaseBuffers();
    void acquireBuffers(uint32_t width, uint32_t height);


private:
    Globals& m_globals;
    GlobalSettings const& m_global_settings;
    Device& m_device;
    dxgi::SwapChain& m_linked_swap_chain;
    RenderingTasks* m_linked_rendering_tasks_ptr;
    bool m_suspend_rendering = false;

    std::vector<Resource> m_color_buffers;
    std::unique_ptr<CommittedResource> m_depth_buffer;

    DXGI_FORMAT m_depth_buffer_native_format;
    ResourceOptimizedClearValue m_depth_optimized_clear_value;
    
    std::vector<RenderingTarget> m_targets;

    std::function<void(RenderingTarget const&)> m_presenter;
};

}

#endif
