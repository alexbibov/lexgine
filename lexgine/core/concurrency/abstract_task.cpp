#include "abstract_task.h"

#include <chrono>

using namespace lexgine::core::concurrency;


AbstractTask::AbstractTask(std::string const& debug_name, bool expose_in_task_graph) :
    m_exposed_in_task_graph{ expose_in_task_graph }
{
    setStringName(debug_name);
}

TaskExecutionStatistics const& AbstractTask::getExecutionStatistics() const
{
    return m_execution_statistics;
}

bool AbstractTask::execute(uint8_t worker_id, uint16_t frame_index)
{
    auto task_begin_execution_time_point = std::chrono::system_clock::now();
    bool result = do_task(worker_id, frame_index);
    auto task_complete_execution_time_point = std::chrono::system_clock::now();

    uint64_t completion_time_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(task_complete_execution_time_point - task_begin_execution_time_point).count();

    m_execution_statistics.worker_id = worker_id;
    m_execution_statistics.execution_time = completion_time_in_ms;

    return result;
}
