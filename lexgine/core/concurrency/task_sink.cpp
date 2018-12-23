#include "task_sink.h"
#include "schedulable_task.h"
#include "../exception.h"

using namespace lexgine::core::concurrency;
using namespace lexgine::core::misc;


class TaskSink::TaskGraphEndExecutionGuard : public SchedulableTask
{
public: 
    TaskGraphEndExecutionGuard(std::atomic_uint16_t& exit_level) :
        SchedulableTask{ "TaskGraphEndExecutionGuard", false },
        m_exit_level{ exit_level }
    {

    }

private:
    std::atomic_uint16_t& m_exit_level;

    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        --m_exit_level;
        return true;
    }

    TaskType get_task_type() const
    {
        return TaskType::cpu;
    }
};




TaskSink::TaskSink(TaskGraph const& source_task_graph, 
    std::vector<std::ostream*> const& worker_thread_logging_streams, std::string const& debug_name):
    m_exit_signal{ false },
    m_exit_level{ 0U },
    m_num_threads_finished{ 0U },
    m_error_watchdog{ 0U },
    m_task_graph_end_execution_guarding_task{ new TaskGraphEndExecutionGuard{m_exit_level} }
{
    setStringName(debug_name);

    m_compiled_task_graph.reset(new TaskGraph{ TaskGraphAttorney<TaskSink>::cloneTaskGraphForFrame(source_task_graph, 0) });
    TaskGraphAttorney<TaskSink>::injectDependentNode(*m_compiled_task_graph, *m_task_graph_end_execution_guarding_task);

    for (uint8_t i = 0; i < source_task_graph.getNumberOfWorkerThreads(); ++i)
    {
        m_workers_list.push_back(std::make_pair(std::thread{}, i < worker_thread_logging_streams.size() ? worker_thread_logging_streams[i] : nullptr));
    }
}

TaskSink::~TaskSink() = default;

void TaskSink::run()
{
    // Start workers
    logger().out("Starting worker threads", LogMessageType::information);
    uint8_t i = 0;
    for (auto worker = m_workers_list.begin(); worker != m_workers_list.end(); ++worker, ++i)
    {
        auto main_log_last_entry_time_stamp = Log::retrieve()->getLastEntryTimeStamp();
        worker->first = std::thread{ &TaskSink::dispatch, this, i, worker->second, main_log_last_entry_time_stamp.getTimeZone(), main_log_last_entry_time_stamp.isDTS() };
        worker->first.detach();
    }

    bool exit_signal;
    uint64_t error_status{ 0x0 };
    while (!(error_status = m_error_watchdog.load(std::memory_order_acquire))
        && (!(exit_signal = m_exit_signal.load(std::memory_order_acquire)) || m_exit_level.load(std::memory_order_acquire) > 0))
    {
        // if exit signal was not dispatched try to start new concurrent frame
        if (!exit_signal && m_exit_level.load(std::memory_order_acquire) == 0)
        {
            TaskGraphAttorney<TaskSink>::resetTaskGraphCompletionStatus(*m_compiled_task_graph);
            ++m_exit_level;
        }

        // try to put more tasks into the task queue
        bool more_tasks_dispatched{ false };
        for (auto& task : *m_compiled_task_graph)
        {
            if (!task.isCompleted() && task.isReadyToLaunch())
            {
                task.schedule(m_task_queue);
                more_tasks_dispatched = true;
            }
        }

        if (!more_tasks_dispatched) YieldProcessor();
    }

    m_task_queue.shutdown();
    while (m_num_threads_finished.load(std::memory_order_acquire) < m_workers_list.size());    // wait until all workers are done with their tasks
    m_num_threads_finished.store(0U, std::memory_order_release);    // reset "job done" semaphore

    // errors may occur at any time during execution
    if (error_status)
    {
        AbstractTask* p_failed_task = reinterpret_cast<AbstractTask*>(error_status);

        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(*this,
            "Task " + p_failed_task->getStringName() + " has failed during execution (" + p_failed_task->getErrorString() + "). Worker thread logs may contain more details");
    }

    logger().out("Main loop finished", LogMessageType::information);
}

void TaskSink::dispatchExitSignal()
{
    m_exit_signal.store(true, std::memory_order_release);
}

void TaskSink::dispatch(uint8_t worker_id, std::ostream* logging_stream, int8_t logging_time_zone, bool logging_dts)
{
    if (logging_stream)
    {
        Log::create(*logging_stream, "Worker #" + std::to_string(worker_id),  logging_time_zone, logging_dts);
        logger().out("Thread " + std::to_string(worker_id) + " log started", LogMessageType::information);
    }

    Optional<TaskGraphNode*> task;
    while (!m_error_watchdog.load(std::memory_order_acquire)
        && ((task = m_task_queue.dequeueTask()).isValid() || !m_exit_signal.load(std::memory_order_acquire) || m_exit_level.load(std::memory_order_acquire) > 0))
    {
        if(task.isValid())
        {
            TaskGraphNode* unwrapped_task = static_cast<TaskGraphNode*>(task);
            AbstractTask* p_contained_task = TaskGraphNodeAttorney<TaskSink>::getContainedTask(*unwrapped_task);
            try
            {
                if (!unwrapped_task->execute(worker_id))
                {
                    // if execution returns 'false', this means that the task has to be rescheduled
                    TaskGraphNodeAttorney<TaskSink>::resetScheduleStatus(*unwrapped_task);
                }
            }
            catch (lexgine::core::Exception const&)
            {
                // we don't do anything here as the logging is done by the tasks themselves via call of raiseError(...)
                // Note that the same call puts the task into erroneous state
            }

            if (p_contained_task->getErrorState())
                m_error_watchdog.store(reinterpret_cast<uint64_t>(p_contained_task), std::memory_order_release);
        }
    }

    m_task_queue.shutdown();

    Log::shutdown();

    ++m_num_threads_finished;
}
