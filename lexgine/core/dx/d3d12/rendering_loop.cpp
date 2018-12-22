#include "lexgine/core/globals.h"

#include "rendering_loop.h"
#include "dx_resource_factory.h"
#include "descriptor_table_builders.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"
#include "device.h"

#include <cassert>

using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;


RenderingLoop::RenderingLoop(Globals& globals,
    std::shared_ptr<RenderingTargetColor> const& rendering_loop_color_target_ptr,
    std::shared_ptr<RenderingTargetDepth> const& rendering_loop_depth_target_ptr) :
    m_device{ *globals.get<Device>() },
    m_queued_frame_counter{ 1U },
    m_color_target_ptr{ rendering_loop_color_target_ptr },
    m_depth_target_ptr{ rendering_loop_depth_target_ptr },
    m_end_of_frame_cpu_wall{ m_device },
    m_end_of_frame_gpu_wall{ m_device },
    m_rendering_tasks{ globals }
{
}

RenderingLoop::~RenderingLoop() = default;

void RenderingLoop::draw()
{
    PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i start", m_queued_frame_counter);

    m_rendering_tasks.run();
    m_end_of_frame_cpu_wall.signalFromCPU();
    m_end_of_frame_gpu_wall.signalFromGPU(m_device.defaultCommandQueue());
   
    PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i finish", m_queued_frame_counter);

    ++m_queued_frame_counter;
}

