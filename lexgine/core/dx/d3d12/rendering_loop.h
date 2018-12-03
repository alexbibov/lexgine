#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_LOOP_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_LOOP_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "descriptor_table_builders.h"
#include "rendering_tasks.h"
#include "resource.h"
#include "resource_barrier_pack.h"

#include "profiler.h"

namespace lexgine::core::dx::d3d12 {

class RenderingLoopColorTarget
{
public:
    RenderingLoopColorTarget(Globals const& globals,
        std::vector<Resource> const& render_targets,
        std::vector<ResourceState> const& render_target_initial_states,
        std::vector<RTVDescriptor> const& render_target_resource_views,
        uint64_t active_color_targets);

    void switchToRenderAccessState(CommandList const& command_list) const;
    void switchToInitialState(CommandList const& command_list) const;

    void setActiveColorTargets(uint64_t active_color_targets_mask);
    uint64_t activeColorTargetsMask() const;

    size_t totalTargetsCount() const;    //! returns total number of color targets
    size_t activeTargetsCount() const;    //! returns number of active color targets

    RenderTargetViewDescriptorTable const& rtvTable() const;

private:
    uint64_t m_active_color_targets;
    DynamicResourceBarrierPack m_forward_barriers;
    DynamicResourceBarrierPack m_backward_barriers;
    RenderTargetViewDescriptorTable m_rtvs_table;
};


class RenderingLoopDepthTarget
{
public:
    RenderingLoopDepthTarget(Globals const& globals,
        Resource const& depth_target_resource,
        ResourceState initial_depth_target_resource_state,
        DSVDescriptor const& depth_target_resource_view);

    void switchToRenderAccessState(CommandList const& command_list) const;
    void switchToInitialState(CommandList const& command_list) const;

    DepthStencilViewDescriptorTable const& dsvTable() const;

private:
    DynamicResourceBarrierPack m_forward_barriers;
    DynamicResourceBarrierPack m_backward_barriers;
    DepthStencilViewDescriptorTable m_dsv_table;
};


class RenderingLoop final
{
public:
    RenderingLoop(Globals& globals, 
        std::shared_ptr<RenderingLoopColorTarget> const& rendering_loop_color_target_ptr,
        std::shared_ptr<RenderingLoopDepthTarget> const& rendering_loop_depth_target_ptr);
    ~RenderingLoop();

    void draw();

    void setFrameBackgroundColor(math::Vector4f const& color);

public:
    class InitFrameTask;
    class PostFrameTask;

private:
    Device& m_device;
    DxResourceFactory const& m_dx_resources;
    GlobalSettings const& m_global_settings;
    uint32_t m_queued_frame_counter;
    std::shared_ptr<RenderingLoopColorTarget> m_color_target_ptr;
    std::shared_ptr<RenderingLoopDepthTarget> m_depth_target_ptr;
    std::vector<Signal> m_frame_end_signals;

    std::unique_ptr<InitFrameTask> m_init_frame_task_ptr;
    std::unique_ptr<PostFrameTask> m_post_frame_task_ptr;
    RenderingTasks m_rendering_tasks;
};

}

#endif    // !LEXGINE_CORE_DX_D3D12_RENDERING_LOOP_H
