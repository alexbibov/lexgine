#include "task_sink.h"
#include "abstract_task.h"
#include "engine/core/exception.h"
#include "engine/core/misc/misc.h"

using namespace lexgine::core::concurrency;
using namespace lexgine::core::misc;

TaskSink::TaskSink(TaskGraph& source_task_graph,
    std::string const& debug_name,
    WorkerEntryHook worker_entry_hook) :
    m_source_task_graph{ source_task_graph },
    m_worker_entry_hook{ std::move(worker_entry_hook) },
    m_task_queue{},
    m_stop_signal{ true },
    m_error_watchdog{ 0 },
    m_work_epoch{ 0 },
    m_submission_finished{ false }
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

    m_stop_signal.store(false, std::memory_order_release);
    m_error_watchdog.store(0, std::memory_order_release);

    // Start worker threads
    uint8_t i = 0;

    for (auto worker = m_workers_list.begin(); worker != m_workers_list.end(); ++worker, ++i)
    {
        assert(!worker->joinable());

        logger().out(misc::formatString("Starting worker thread %i", i), LogMessageType::information);
        *worker = std::thread{ &TaskSink::dispatch, this, i };

        #ifdef _WIN32
        HANDLE threadNativeHandle = worker->native_handle();
        SetThreadDescription(threadNativeHandle,std::format(L"{} thread #{}", asciiStringToWstring(getStringName()), i).c_str());
        #endif
    }
}

void TaskSink::submit(uint64_t user_data)
{
    assert(!m_stop_signal.load(std::memory_order_acquire));

    m_source_task_graph.setUserData(user_data);

    {
        std::lock_guard lock{ m_frame_completion_mutex };
        m_submission_finished = false;
    }

    // the rest of the graph is scheduled by the workers as the dependencies of each node are met
    for (auto& task : m_source_task_graph)
    {
        if (!task.unmetDependencyCount()) task.schedule(m_task_queue);
    }

    notifyWorkAvailable();

    {
        std::unique_lock lock{ m_frame_completion_mutex };
        m_frame_completion_signal.wait(lock,
            [this]
            {
                return m_submission_finished || m_error_watchdog.load(std::memory_order_acquire);
            });
    }

    uint64_t error_status = m_error_watchdog.load(std::memory_order_acquire);

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
    notifyWorkAvailable();

    for (auto& worker : m_workers_list)
    {
        if (worker.joinable()) worker.join();
    }
    m_task_queue.shutdown();

    logger().out("Worker threads finished", LogMessageType::information);
}

bool TaskSink::isRunning() const
{
    return !m_stop_signal.load(std::memory_order_acquire);
}


void TaskSink::waitForWork(uint64_t observed_work_epoch)
{
    std::unique_lock lock{ m_work_availability_mutex };
    m_work_availability_signal.wait(lock,
        [this, observed_work_epoch]
        {
            return m_work_epoch.load(std::memory_order_acquire) != observed_work_epoch
                || m_stop_signal.load(std::memory_order_acquire)
                || m_error_watchdog.load(std::memory_order_acquire);
        });
}

void TaskSink::notifyWorkAvailable()
{
    {
        std::lock_guard lock{ m_work_availability_mutex };
        m_work_epoch.fetch_add(1, std::memory_order_release);
    }

    m_work_availability_signal.notify_all();
}

void TaskSink::notifyFrameFinished()
{
    {
        std::lock_guard lock{ m_frame_completion_mutex };
        m_submission_finished = true;
    }

    m_frame_completion_signal.notify_all();
}

void TaskSink::dispatch(uint8_t worker_id)
{
    if (m_worker_entry_hook) m_worker_entry_hook(worker_id);

    logger().out(misc::formatString("###### Worker thread %i log start ######", worker_id), LogMessageType::information);

    while (!m_error_watchdog.load(std::memory_order_acquire))
    {
        uint64_t const observed_work_epoch = m_work_epoch.load(std::memory_order_acquire);
        Optional<TaskGraphNode*> task = m_task_queue.dequeueTask();

        if (!task.isValid())
        {
            if (m_stop_signal.load(std::memory_order_acquire)) break;

            waitForWork(observed_work_epoch);
            continue;
        }

        TaskGraphNode* unwrapped_task = *task;
        AbstractTask* p_contained_task = unwrapped_task->task();
        bool task_was_completed{ false };
        try
        {
            task_was_completed = unwrapped_task->execute(worker_id);
            if (!task_was_completed)
            {
                // if execution returns 'false', this means that the task has to be rescheduled. Its dependencies
                // stay met, so the node is put back into the queue instead of awaiting them anew
                unwrapped_task->resetForReexecution();
                unwrapped_task->schedule(m_task_queue);
                notifyWorkAvailable();
            }
        }
        catch (lexgine::core::Exception const&)
        {
            // we don't do anything here as the logging is done by the tasks themselves via call of raiseError(...)
            // Note that the same call puts the task into erroneous state
        }

        if (p_contained_task->getErrorState())
        {
            m_error_watchdog.store(reinterpret_cast<uint64_t>(p_contained_task), std::memory_order_release);
            notifyWorkAvailable();
            notifyFrameFinished();
        }
        else if (task_was_completed)
        {
            if (unwrapped_task->scheduleReadyDependents(m_task_queue)) notifyWorkAvailable();

            if (TaskGraphAttorney<TaskSink>::isTaskGraphCompleted(m_source_task_graph)) notifyFrameFinished();
        }
    }

    m_task_queue.shutdown();
    logger().out(misc::formatString("###### Worker thread %i log end ######", worker_id), LogMessageType::information);
}
