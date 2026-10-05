#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task_graph.h"

#include <condition_variable>
#include <mutex>
#include <vector>

namespace lexgine::core::concurrency {

//! Implements task scheduling based on provided task graph
class TaskSink final : public NamedEntity<TaskSink>
{
public:
    TaskSink(
        TaskGraph& source_task_graph,
        std::string const& debug_name = "");

    ~TaskSink();

    void start();    //! starts the task sink

    /*! executes the task graph associated with the sink using @param user_data 
     to forward arbitrary user data to tasks in the source task graph.
    */
    void submit(uint64_t user_data);
    void shutdown();    //! shutdowns the sink
    bool isRunning() const;    //! returns 'true' if the task sink is running

private:
    void dispatch(uint8_t worker_id);    //! function looped by worker threads
    void waitForWork(uint64_t observed_work_epoch);    //! parks the calling worker until work is enqueued, the sink is stopped or a task fails
    void notifyWorkAvailable();    //! wakes up the workers parked in waitForWork
    
private:
    TaskGraph& m_source_task_graph;    //!< the task graph executed by the sink

    std::vector<std::thread> m_workers_list;    //!< vector of worker threads
    RingBufferTaskQueue<TaskGraphNode*> m_task_queue;    //!< concurrent task queue


    std::atomic_bool m_stop_signal;    //!< acquires 'true' when the sink is to be stopped

    /*!< equals 0 if all tasks have been completed without errors. Acquires a non-zero value otherwise. 
     The value acquired in the latter case contains a pointer to the task graph node was the source of the error.
    */
    std::atomic_uint64_t m_error_watchdog;

    std::mutex m_work_availability_mutex;    //!< guards the updates of m_work_epoch against the workers about to park
    std::condition_variable m_work_availability_signal;    //!< signalled whenever the workers may have something to do
    std::atomic_uint64_t m_work_epoch;    //!< incremented on every event that may unblock a parked worker
};

}

#endif    // LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
