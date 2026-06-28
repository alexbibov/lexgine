#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/device.h"

#include "submesh_rendering_task.h"


namespace lexgine::core::dx::d3d12::tasks::rendering_tasks
{

SubmeshRenderingTask::SubmeshRenderingTask(Globals& globals, BasicRenderingServices& rendering_services)
    : RenderingWork{ globals, "Submesh rendering task", CommandType::direct }
    , m_device{ *globals.get<Device>() }
    , m_basic_rendering_services{ rendering_services }
{

}


void SubmeshRenderingTask::updateRenderingConfiguration(
    RenderingConfigurationUpdateFlags update_flags,
    RenderingConfiguration const& rendering_configuration
)
{

}


bool SubmeshRenderingTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    return true;
}

}