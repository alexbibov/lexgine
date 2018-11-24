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

class RenderingLoopTarget
{
public:
    RenderingLoopTarget(std::vector<Resource> const& target_resources,
        std::vector<ResourceState> const& target_resources_initial_states);

    void switchToRenderAccessState(CommandList const& command_list) const;
    void switchToInitialState(CommandList const& command_list) const;

private:
    std::vector<Resource> m_target_resources;
    DynamicResourceBarrierPack m_forward_barriers;
    DynamicResourceBarrierPack m_backward_barriers;
};


class RenderingLoop
{
public:
    RenderingLoop(Globals const& globals, 
        std::shared_ptr<RenderingLoopTarget> const& rendering_loop_target_ptr);

    void draw();

private:
    Device const& m_device;
    GlobalSettings const& m_global_settings;
    uint32_t m_queued_frame_counter;
    std::shared_ptr<RenderingLoopTarget> m_rendering_loop_target_ptr;
    std::vector<Signal> m_frame_end_signals;
    RenderTargetViewDescriptorTableReference m_rtvs;
    RenderingTasks m_rendering_tasks;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_LOOP_H
