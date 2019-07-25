#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_WORK_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_WORK_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/misc/flags.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/viewport.h"

#include "lexgine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"

namespace lexgine::core::dx::d3d12 {

struct RenderingConfiguration
{
    Viewport viewport;
    DXGI_FORMAT color_buffer_format;
    DXGI_FORMAT depth_buffer_format;
    osinteraction::windows::Window* p_rendering_window;
};

class RenderingWork : public concurrency::SchedulableTask
{
public:
    BEGIN_FLAGS_DECLARATION(RenderingConfigurationUpdateFlags)
        FLAG(viewport_changed, 0x1)
        FLAG(color_format_changed, 0x2)
        FLAG(depth_format_changed, 0x4)
        FLAG(rendering_window_changed, 0x8)
        END_FLAGS_DECLARATION(RenderingConfigurationUpdateFlags)

public:
    RenderingWork(std::string const& debug_name, std::unique_ptr<ProfilingService>&& profiling_service = nullptr)
        : SchedulableTask{ debug_name, true, std::move(profiling_service) }
    {

    }

    virtual void updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags,
        RenderingConfiguration const& rendering_configuration) = 0;
};


}

#endif
