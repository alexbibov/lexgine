#ifndef LEXGINE_CORE_DX_D3D12_SWAP_CHAIN_LINK_H
#define LEXGINE_CORE_DX_D3D12_SWAP_CHAIN_LINK_H

#include <optional>
#include <vector>
#include <thread>
#include <functional>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"

#include "frame_pacer.h"
#include "render_output.h"


namespace lexgine::core::dx::d3d12 {

//! Implements link between swap chain and main rendering loop
class SwapChainLink final
{
public:
    static std::shared_ptr<SwapChainLink> create(Globals& globals, dxgi::SwapChain& swap_chain_to_link,
        SwapChainDepthBufferFormat depth_buffer_format);

public:
    ~SwapChainLink();

    SwapChainLink(SwapChainLink const&) = delete;
    SwapChainLink(SwapChainLink&& other);

    void linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link);

    //! renders a single frame. Returns 'false' when the output had nothing to present this iteration
    bool render();

    void flushRenderingTasks();    //! awaits the GPU work scheduled so far; called by the thread driving the frames

    //! records a request to resize the output; the surfaces are recreated by the next rendered frame
    void requestResize(uint32_t new_width, uint32_t new_height);

    //! stops the frames from being rendered until the output has a client area to render into again
    void suspendRendering();

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
    std::optional<FramePacer> m_frame_pacer;    //!< available once the rendering tasks owning the frame progress tracker are linked
};

}

#endif
