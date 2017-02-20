#include "task_sink.h"

using namespace lexgine::core::concurrency;
using namespace lexgine::core::misc;

TaskSink::TaskSink(std::string const& debug_name, TaskGraph const& source_task_graph, std::vector<std::ostream*> const& worker_thread_logging_streams):
    m_source_tasks{ TaskGraphAttorney<TaskSink>::getTaskList(source_task_graph) },
    m_exit_signal{ false }
{
    setStringName(debug_name);
}

TaskSink::TaskSink(TaskGraph const& source_task_graph, std::vector<std::ostream*> const& worker_thread_logging_streams):
    m_source_tasks{ TaskGraphAttorney<TaskSink>::getTaskList(source_task_graph) },
    m_exit_signal{ false }
{
    setStringName("DefaultTaskSink");

    for (uint8_t i = 0; i < source_task_graph.getNumberOfWorkerThreads(); ++i)
    {
        m_workers_list.push_back(std::make_pair(std::thread{}, worker_thread_logging_streams.size() > i ? worker_thread_logging_streams[i] : nullptr));
    }
}

void TaskSink::run(uint16_t max_frames_queued)
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


    std::list<AbstractTask*> task_flow{};
    while(!m_exit_signal)
    {
        if (loop_tasks)
            task_flow.insert(task_flow.end(), m_task_list.begin(), m_task_list.end());


        auto current_task = task_flow.begin();
        while(current_task != task_flow.end())
        {
            if ((*current_task)->isReadyToLaunch())
            {
                m_task_queue.enqueueTask(*current_task);

                auto task_to_remove = current_task;
                ++current_task;
                task_flow.erase(task_to_remove);
            }
            else
            {
                ++current_task;
            }
        }
    }
}

void TaskSink::dispatch(uint8_t worker_id, std::ostream* logging_stream, int8_t logging_time_zone, bool logging_dts)
{
    if (logging_stream)
    {
        Log::create(*logging_stream, logging_time_zone, logging_dts);
        Log::retrieve()->out("Thread " + std::to_string(worker_id) + " log started");
    }

    Optional<AbstractTask*> task;
    while (!m_exit_signal || (task = m_task_queue.dequeueTask()).isValid())
    {
        if(task.isValid())
        {
            AbstractTask* unwrapped_task = static_cast<AbstractTask*>(task);
            unwrapped_task->execute(worker_id);

            if (AbstractTaskAttorney<TaskSink>::getTaskType(*unwrapped_task) == TaskType::exit
                && unwrapped_task->isCompleted())
            {
                m_exit_signal = true;
            }
        }
    }

    Log::shutdown();
}
