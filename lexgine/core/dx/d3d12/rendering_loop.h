#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_LOOP_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_LOOP_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "descriptor_table_builders.h"
#include "rendering_tasks.h"
#include "resource.h"
#include "resource_barrier_pack.h"
#include "rendering_target.h"
#include "signal.h"

#include "profiler.h"

namespace lexgine::core::dx::d3d12 {

class RenderingLoop final
{
public:
    RenderingLoop(Globals& globals, 
        std::shared_ptr<RenderingTargetColor> const& rendering_loop_color_target_ptr,
        std::shared_ptr<RenderingTargetDepth> const& rendering_loop_depth_target_ptr);
    ~RenderingLoop();

    void draw();

private:
    Device& m_device;
    uint64_t m_queued_frame_counter;
    std::shared_ptr<RenderingTargetColor> m_color_target_ptr;
    std::shared_ptr<RenderingTargetDepth> m_depth_target_ptr;
    Signal m_end_of_frame_cpu_wall;
    Signal m_end_of_frame_gpu_wall;
    RenderingTasks m_rendering_tasks;
};

}

#endif    // !LEXGINE_CORE_DX_D3D12_RENDERING_LOOP_H
