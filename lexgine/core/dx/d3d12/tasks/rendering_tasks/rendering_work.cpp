#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/profiling_services.h"
#include "lexgine/core/dx/d3d12/device.h"

#include "rendering_work.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;


RenderingWork::RenderingWork(Globals& globals, std::string const& debug_name,
    CommandType command_type, bool enable_profiling /* = true */)
    : SchedulableTask{ debug_name }
    , m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_command_type{ command_type }
{
    GlobalSettings& global_settings = *globals.get<GlobalSettings>();
    if (enable_profiling)
    {
        addProfilingService(std::make_unique<CPUTaskProfilingService>(global_settings, debug_name + " CPU execution time"));
        addProfilingService(std::make_unique<GPUTaskProfilingService>(global_settings, debug_name + " GPU execution time"))
            ->assignCommandLists(m_cmd_lists, m_device, m_command_type);
    }
    else
    {

    }
    
}

CommandList* RenderingWork::addCommandList(uint32_t node_mask/* = 0x1*/,
    FenceSharing command_list_sync_mode/* = FenceSharing::none*/,
    PipelineState const* initial_pipeline_state/* = nullptr*/)
{
    m_cmd_lists.emplace_back(m_device.createCommandList(m_command_type, node_mask, command_list_sync_mode, initial_pipeline_state));
    CommandList& rv = m_cmd_lists.back();
    return &rv;
}
