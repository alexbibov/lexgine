#include "task_sink.h"

using namespace lexgine::core::concurrency;
using namespace lexgine::core::misc;

TaskSink::TaskSink(TaskGraph const& source_task_graph):
    m_task_list{ TaskGraphAttorney<TaskSink>::getTaskList(source_task_graph) },
    m_exit_signal{ false }
{
    for (uint8_t i = 0; i < source_task_graph.getNumberOfWorkerThreads(); ++i)
        m_workers_list.push_back(std::unique_ptr<std::thread>{ new std::thread{ &TaskSink::dispatch, this, i } });
}

void TaskSink::run()
{
    // Start workers
    Log::retrieve()->out("Starting worker threads");
    for (auto& worker : m_workers_list)
    {
        worker->detach();
    }


    std::list<AbstractTask*> task_flow{};
    while (!m_exit_signal)
    {
        task_flow.insert(task_flow.end(), m_task_list.begin(), m_task_list.end());

        for (auto task = task_flow.begin(); task != task_flow.end(); ++task)
        {
            if ((*task)->isReadyToLaunch())
            {
                m_task_queue.enqueueTask(*task);
                auto prev_task = --task;
                task_flow.erase(task);
                task = prev_task;
            }
        }
    }
}

void TaskSink::dispatch(uint8_t worker_id)
{
    Log::retrieve()->out("Initializing worker thread " + std::to_string(worker_id));

    Optional<AbstractTask*> task;
    while (!(task = m_task_queue.dequeueTask()).isValid())
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
