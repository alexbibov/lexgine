#ifndef LEXGINE_CORE_UI_PROFILER_H
#define LEXGINE_CORE_UI_PROFILER_H

#include <chrono>
#include <unordered_map>

#include "3rd_party/imgui/imgui.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/ui/ui_provider.h"
#include "lexgine/core/misc/static_vector.h"


namespace lexgine::core::ui {

class Profiler : public UIProvider
{

public:
    static std::shared_ptr<Profiler> create(Globals const& globals, concurrency::TaskGraph const& task_graph)
    {
        return std::shared_ptr<Profiler>{new Profiler{ globals, task_graph }};
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
    Profiler(Globals const& globals, concurrency::TaskGraph const& task_graph);

    double getWorkloadTimePerFrame(ProfilingServiceType workload_type) const;
    double getCPUTimePerFrame() const;
    double getGPUTimePerFrame() const;
    double getFrameTime() const;
    double getFPS() const;

private:
    Globals const& m_globals;
    concurrency::TaskGraph const& m_task_graph;
    bool m_show_profiler;
    std::unordered_map<concurrency::AbstractTask*, std::vector<ProfilingSummary>> m_profiling_summaries;
    misc::StaticVector<double, static_cast<size_t>(ProfilingServiceType::count)> m_total_times;
};

}

#endif
