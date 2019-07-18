#include <chrono>

#include "abstract_task.h"
#include "lexgine/core/dx/d3d12/pix_support.h"

using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::concurrency;


namespace {

AbstractTask::profiler_timer_resolution_t getCurrentTime()
{
    return std::chrono::duration_cast<AbstractTask::profiler_timer_resolution_t>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

}



AbstractTask::AbstractTask(bool enable_profiling, std::string const& debug_name, bool expose_in_task_graph)
    : m_enable_profiling{ enable_profiling }
    , m_exposed_in_task_graph{ expose_in_task_graph }
    , m_task_cpu_profiling_last_tick{ getCurrentTime() }
    , m_task_cpu_profiler_spin_counter{ 0U }
    , m_execution_statistics{ {}, nullptr }
{
    if (debug_name.length())
        setStringName(debug_name);
}

AbstractTask::execution_statistics_t const& AbstractTask::getExecutionStatistics() const
{
    return m_execution_statistics;
}

bool AbstractTask::execute(uint8_t worker_id, uint64_t user_data)
{
    if (m_enable_profiling)
    {
        PIXBeginEvent(pix_marker_colors::PixCPUJobMarkerColor, getStringName().c_str());
        m_task_cpu_profiling_last_tick = getCurrentTime();
    }

    bool result = doTask(worker_id, user_data);

    if (m_enable_profiling)
    {
        auto time_delta = getCurrentTime() - m_task_cpu_profiling_last_tick;

        auto& statistics = m_execution_statistics.statistics;
        if (m_task_cpu_profiler_spin_counter < c_profiler_statistics_package_length)
        {
            statistics[m_task_cpu_profiler_spin_counter++] = time_delta;
        }
        else
        {
            for (int i = 0; i < c_profiler_statistics_package_length - 1; ++i)
                statistics[i] = statistics[i + 1];
            statistics[c_profiler_statistics_package_length - 1] = time_delta;
        }

        PIXEndEvent();
    }

    return result;
}
