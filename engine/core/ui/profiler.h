#ifndef LEXGINE_CORE_UI_PROFILER_H
#define LEXGINE_CORE_UI_PROFILER_H

#include <chrono>
#include <unordered_map>

#include "imgui.h"
#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "engine/core/ui/ui_provider.h"
#include "engine/core/misc/static_vector.h"


namespace lexgine::core::ui {

class Profiler : public UIProvider
{

public:
    static std::shared_ptr<Profiler> create(Globals const& globals, 
        dx::d3d12::BasicRenderingServices const& basic_rendering_services,
        concurrency::TaskGraph const& task_graph)
    {
        return std::shared_ptr<Profiler>{new Profiler{ globals, basic_rendering_services, task_graph }};
    }

public:    // required by UIProvider
    void constructUI() override;
    void toggleEnableState() { m_show_profiler = !m_show_profiler; }

private:
    struct ProfilingSummary final
    {
        std::string name;    //!< name of the source profiling service
        double average_execution_time_ms;    //!< average execution time of a task represented in milliseconds
        uint32_t group_id;    //!< numerical value identifying the type of source profiling service
    };

private:
    Profiler(Globals const& globals, 
        dx::d3d12::BasicRenderingServices const& basic_rendering_services,
        concurrency::TaskGraph const& task_graph);

    double getWorkloadTimePerFrame(ProfilingServiceType workload_type) const;
    double getCPUTimePerFrame() const;
    double getGPUTimePerFrame() const;
    double getFrameTime() const;
    double getFPS() const;

private:
    Globals const& m_globals;
    dx::d3d12::BasicRenderingServices const& m_basic_rendering_services;
    concurrency::TaskGraph const& m_task_graph;
    dx::d3d12::QueryCache const& m_query_cache;
    bool m_show_profiler;
    std::unordered_map<concurrency::AbstractTask*, std::vector<ProfilingSummary>> m_profiling_summaries;
    misc::StaticVector<double, static_cast<size_t>(ProfilingServiceType::count)> m_total_times;
};

}

#endif
