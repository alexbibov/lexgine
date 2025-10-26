#include "task_sink.h"
#include "abstract_task.h"
#include "engine/core/exception.h"
#include "engine/core/misc/misc.h"

using namespace lexgine::core::concurrency;
using namespace lexgine::core::misc;

TaskSink::TaskSink(TaskGraph& source_task_graph,
    std::string const& debug_name) :
    m_source_task_graph{ source_task_graph },
    m_task_queue{},
    m_num_threads_finished{ source_task_graph.getNumberOfWorkerThreads() },
    m_stop_signal{ true },
    m_error_watchdog{ 0 }
{
    m_workers_list.resize(source_task_graph.getNumberOfWorkerThreads());
    setStringName(debug_name);
}

TaskSink::~TaskSink()
{
    if (!m_stop_signal.load(std::memory_order_acquire))
    {
        shutdown();
    }
}

void TaskSink::start()
{
    assert(m_stop_signal.load(std::memory_order_acquire));

    logger().out(misc::formatString("Starting task sink %s", getStringName().c_str()), LogMessageType::information);

    TaskGraphAttorney<TaskSink>::compileTaskGraph(m_source_task_graph);

    m_num_threads_finished.store(0, std::memory_order_release);
    m_stop_signal.store(false, std::memory_order_release);
    m_error_watchdog.store(0, std::memory_order_release);

    // Start worker threads
    uint8_t i = 0;

    for (auto worker = m_workers_list.begin(); worker != m_workers_list.end(); ++worker, ++i)
    {
        logger().out(misc::formatString("Starting worker thread %i", i), LogMessageType::information);
        *worker = std::thread{ &TaskSink::dispatch, this, i };

        #ifdef _WIN32
        HANDLE threadNativeHandle = worker->native_handle();
        SetThreadDescription(threadNativeHandle, (L"Rendering thread #" + std::to_wstring(i)).c_str());
        #endif

        worker->detach();
    }
}

void TaskSink::submit(uint64_t user_data)
{
    assert(!m_stop_signal.load(std::memory_order_acquire));

    uint64_t error_status{ 0x0 };
    m_source_task_graph.setUserData(user_data);

    while (!TaskGraphAttorney<TaskSink>::isTaskGraphCompleted(m_source_task_graph)
        && !(error_status = m_error_watchdog.load(std::memory_order_acquire)))
    {
        bool some_tasks_were_dispatched{ false };
        uint32_t yield_counter{ 0U };

        // try to put more tasks into the task queue
        for (auto& task : m_source_task_graph)
        {
            if (!task.isCompleted() && task.isReadyToLaunch())
            {
                task.schedule(m_task_queue);
                some_tasks_were_dispatched = true;
                yield_counter = 0;
            }
        }

        if (!some_tasks_were_dispatched)
        {
            std::this_thread::yield();
            ++yield_counter;

            if (yield_counter >= 60)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // errors may occur at any time during execution
    if (error_status)
    {
        AbstractTask* p_failed_task = reinterpret_cast<AbstractTask*>(error_status);

        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(*this,
            "Task " + p_failed_task->getStringName() + " has failed during execution (" + p_failed_task->getErrorString() + "). Worker thread logs may contain more details");
    }

    // reset task graph completion status
    m_source_task_graph.resetExecutionStatus();
}

void TaskSink::shutdown()
{
    assert(!m_stop_signal.load(std::memory_order_acquire));

    logger().out(misc::formatString("Task sink %s is shutting down", getStringName().c_str()), LogMessageType::information);

    m_stop_signal.store(true, std::memory_order_release);    //! dispatch the stop signal
    while (m_num_threads_finished.load(std::memory_order_acquire) < m_workers_list.size()) YieldProcessor();
    m_task_queue.shutdown();

    logger().out("Worker threads finished", LogMessageType::information);
}

bool TaskSink::isRunning() const
{
    return !m_stop_signal.load(std::memory_order_acquire);
}


void TaskSink::dispatch(uint8_t worker_id)
{
    logger().out(misc::formatString("###### Worker thread %i log start ######", worker_id), LogMessageType::information);

    Optional<TaskGraphNode*> task;
    while (!m_error_watchdog.load(std::memory_order_acquire)
        && ((task = m_task_queue.dequeueTask()).isValid() || !m_stop_signal.load(std::memory_order_acquire)))
    {
        if (task.isValid())
        {
            TaskGraphNode* unwrapped_task = *task;
            AbstractTask* p_contained_task = unwrapped_task->task();
            try
            {
                if (!unwrapped_task->execute(worker_id))
                {
                    // if execution returns 'false', this means that the task has to be rescheduled
                    unwrapped_task->resetExecutionStatus();
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
        else
        {
            std::this_thread::yield();
        }
    }

    m_task_queue.shutdown();
    logger().out(misc::formatString("###### Worker thread %i log end ######", worker_id), LogMessageType::information);

    ++m_num_threads_finished;
}
