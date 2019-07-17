#include <chrono>

#include "abstract_task.h"
#include "lexgine/core/dx/d3d12/pix_support.h"

using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::concurrency;


namespace {

ExecutionStatistics::time_resolution_t getCurrentTime()
{
    return std::chrono::duration_cast<ExecutionStatistics::time_resolution_t>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

}

ExecutionStatistics::ExecutionStatistics()
    : m_last_tick{ getCurrentTime() }
    , m_spin_up_counter{ 0 }
    , m_worker_id{ 0 }
{
}

void ExecutionStatistics::tick(uint8_t worker_id)
{
    m_last_tick = getCurrentTime();
    m_worker_id = worker_id;
}

void ExecutionStatistics::tock()
{
    auto time_delta = getCurrentTime() - m_last_tick;

    if (m_spin_up_counter < m_statistics.size())
    {
        m_statistics[m_spin_up_counter++] = time_delta;
    }
    else
    {
        for (int i = 0; i < m_statistics.size() - 1; ++i)
            m_statistics[i] = m_statistics[i + 1];
        m_statistics[m_statistics.size() - 1] = time_delta;
    }
}



AbstractTask::AbstractTask(std::string const& debug_name, bool expose_in_task_graph) :
    m_exposed_in_task_graph{ expose_in_task_graph }
{
    if(debug_name.length())
        setStringName(debug_name);
}

ExecutionStatistics const& AbstractTask::getExecutionStatistics() const
{
    return m_execution_statistics;
}

bool AbstractTask::execute(uint8_t worker_id, uint64_t user_data)
{
    
    PIXBeginEvent(pix_marker_colors::PixCPUJobMarkerColor, getStringName().c_str());
    m_execution_statistics.tick(worker_id);

    bool result = doTask(worker_id, user_data);

    m_execution_statistics.tock();
    PIXEndEvent();

    return result;
}
