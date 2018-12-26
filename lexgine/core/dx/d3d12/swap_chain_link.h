#ifndef LEXGINE_CORE_DX_D3D12_SWAP_CHAIN_LINK_H
#define LEXGINE_CORE_DX_D3D12_SWAP_CHAIN_LINK_H

#include <vector>
#include <thread>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"

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
class SwapChainLink final
{
public:
    SwapChainLink(Globals& globals, dxgi::SwapChain const& swap_chain_to_link,
        SwapChainDepthBufferFormat depth_buffer_format);
    ~SwapChainLink();

    SwapChainLink(SwapChainLink const&) = delete;
    SwapChainLink(SwapChainLink&&) = default;

    void linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link);

    void beginRenderingLoop();

    void dispatchExitSignal();

    void present();

private:
    Globals& m_globals;
    GlobalSettings const& m_global_settings;
    Device& m_device;
    dxgi::SwapChain const& m_linked_swap_chain;
    RenderingTasks* m_linked_rendering_tasks_ptr;

    std::unique_ptr<Heap> m_depth_buffer_heap;
    DXGI_FORMAT m_depth_buffer_native_format;
    ResourceOptimizedClearValue m_depth_optimized_clear_value;
    Resource m_depth_buffer;

    std::vector<std::shared_ptr<RenderingTarget>> m_targets;

    uint64_t m_expected_frame_index;

    std::thread m_rendering_tasks_producer_thread;
};

}

#endif
