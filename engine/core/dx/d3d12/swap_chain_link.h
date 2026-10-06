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

#include "render_output.h"


namespace lexgine::core::dx::d3d12 {

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


private:
    Globals& m_globals;
    GlobalSettings const& m_global_settings;
    dxgi::SwapChain& m_linked_swap_chain;
    RenderingTasks* m_linked_rendering_tasks_ptr;
    bool m_suspend_rendering = false;

    RenderOutput m_render_output;
};

}

#endif
