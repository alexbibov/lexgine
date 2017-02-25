#include "task_sink.h"
#include "schedulable_task.h"

using namespace lexgine::core::concurrency;
using namespace lexgine::core::misc;


class TaskSink::TaskGraphEndExecutionGuard : public SchedulableTask
{
public: 
    TaskGraphEndExecutionGuard(std::vector<std::atomic_bool>& guard_vector, std::atomic_uint16_t& exit_level) :
        SchedulableTask{ "TaskGraphEndExecutionGuard", false },
        m_guard_vector{guard_vector},
        m_exit_level{ exit_level }
    {

    }

private:
    std::vector<std::atomic_bool>& m_guard_vector;
    std::atomic_uint16_t& m_exit_level;

    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        m_guard_vector[frame_index].store(false, std::memory_order_release);
        --m_exit_level;
        return true;
    }

    TaskType get_type() const
    {
        return TaskType::cpu;
    }
};




TaskSink::TaskSink(TaskGraph const& source_task_graph, std::vector<std::ostream*> const& worker_thread_logging_streams, uint16_t max_frames_to_queue, std::string const& debug_name):
    m_task_graphs(max_frames_to_queue),
    m_task_graph_execution_busy_vector(max_frames_to_queue),
    m_exit_signal{ false },
    m_exit_level{ 0U },
    m_task_graph_end_execution_guarding_task{ new TaskGraphEndExecutionGuard{m_task_graph_execution_busy_vector, m_exit_level} }
{
    setStringName(debug_name);

    for (uint8_t i = 0; i < source_task_graph.getNumberOfWorkerThreads(); ++i)
    {
        m_workers_list.push_back(std::make_pair(std::thread{}, i < worker_thread_logging_streams.size() ? worker_thread_logging_streams[i] : nullptr));
    }

    for (uint16_t i = 0; i < max_frames_to_queue; ++i)
    {
        m_task_graphs[i].setRootNodes(TaskGraphAttorney<TaskSink>::getTaskGraphRootNodeList(source_task_graph));
        TaskGraphAttorney<TaskSink>::setTaskGraphFrameIndex(m_task_graphs[i], i);
        m_task_graphs[i].injectDependentTask(*m_task_graph_end_execution_guarding_task);
        std::atomic_init(&m_task_graph_execution_busy_vector[i], false);
    }
}

TaskSink::~TaskSink() = default;

void TaskSink::run()
{
    // Start workers
    Log::retrieve()->out("Starting worker threads");
    uint8_t i = 0;
    for (auto worker = m_workers_list.begin(); worker != m_workers_list.end(); ++worker, ++i)
    {
        auto main_log_last_entry_time_stamp = Log::retrieve()->getLastEntryTimeStamp();
        worker->first = std::thread{ &TaskSink::dispatch, this, i, worker->second, main_log_last_entry_time_stamp.getTimeZone(), main_log_last_entry_time_stamp.isDTS() };
        worker->first.detach();
    }

    bool exit_signal;
    while (!(exit_signal = m_exit_signal.load(std::memory_order_acquire)) || m_exit_level.load(std::memory_order_acquire) > 0)
    {
        for (uint16_t i = 0; i < m_task_graph_execution_busy_vector.size(); ++i)
        {
            // if exit signal was not dispatched try to start new concurrent frame 
            register bool false_val = false;
            if (!exit_signal && m_task_graph_execution_busy_vector[i].compare_exchange_strong(false_val, true, std::memory_order_acq_rel))
            {
                TaskGraphAttorney<TaskSink>::resetTaskGraphCompletionStatus(m_task_graphs[i]);
                ++m_exit_level;
            }

            // try to put more tasks into the task queue
            for (auto task : m_task_graphs[i])
            {
                if (!task->isCompleted() && task->isReadyToLaunch())
                    task->schedule(m_task_queue);
            }
        }
    }

    Log::retrieve()->out("Main loop finished");
}

void TaskSink::dispatchExitSignal()
{
    m_exit_signal.store(true, std::memory_order_release);
}

void TaskSink::dispatch(uint8_t worker_id, std::ostream* logging_stream, int8_t logging_time_zone, bool logging_dts)
{
    if (logging_stream)
    {
        Log::create(*logging_stream, logging_time_zone, logging_dts);
        Log::retrieve()->out("Thread " + std::to_string(worker_id) + " log started");
    }

    Optional<TaskGraphNode*> task;
    while ((task = m_task_queue.dequeueTask()).isValid() || !m_exit_signal.load(std::memory_order_acquire) || m_exit_level.load(std::memory_order_acquire) > 0)
    {
        if(task.isValid())
        {
            TaskGraphNode* unwrapped_task = static_cast<TaskGraphNode*>(task);
            unwrapped_task->execute(worker_id);
        }
    }

    Log::shutdown();
}
