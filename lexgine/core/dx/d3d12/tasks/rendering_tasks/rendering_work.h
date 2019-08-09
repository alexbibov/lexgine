#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_RENDERING_WORK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_RENDERING_WORK_H

#include <list>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/viewport.h"
#include "lexgine/core/misc/flags.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/dx/d3d12/command_list.h"
#include "lexgine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"

#include "lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {



template<typename T> class RenderingWorkAttorney;

class RenderingWork : public concurrency::SchedulableTask
{
    friend class RenderingWorkAttorney<GpuWorkExecutionTask>;

public:
    BEGIN_FLAGS_DECLARATION(RenderingConfigurationUpdateFlags)
        FLAG(viewport_changed, 0x1)
        FLAG(color_format_changed, 0x2)
        FLAG(depth_format_changed, 0x4)
        FLAG(rendering_window_changed, 0x8)
        END_FLAGS_DECLARATION(RenderingConfigurationUpdateFlags)

public:
    RenderingWork(Globals& globals, std::string const& debug_name,
        CommandType command_type, bool enable_profiling = true);

    virtual void updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags,
        RenderingConfiguration const& rendering_configuration) = 0;

    CommandList* addCommandList(uint32_t node_mask = 0x1,
        FenceSharing command_list_sync_mode = FenceSharing::none,
        PipelineState const* initial_pipeline_state = nullptr);

private:
    Globals& m_globals;
    Device& m_device;
    CommandType m_command_type;
    std::list<CommandList> m_cmd_lists;
};

template<> class RenderingWorkAttorney<GpuWorkExecutionTask>
{
    friend class GpuWorkExecutionTask;

    static CommandType renderingWorkCommandType(RenderingWork const& rendering_work)
    {
        return rendering_work.m_command_type;
    }

    static std::list<CommandList>& renderingWorkCommands(RenderingWork& rendering_work)
    {
        return rendering_work.m_cmd_lists;
    }

};

}

#endif
