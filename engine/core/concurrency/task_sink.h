#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task_graph.h"
#include "engine/core/class_names.h"

#include <vector>

namespace lexgine::core::concurrency {

//! Implements task scheduling based on provided task graph
class TaskSink final : public NamedEntity<class_names::TaskSink>
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
    
private:
    TaskGraph& m_source_task_graph;    //!< the task graph executed by the sink

    std::vector<std::thread> m_workers_list;    //!< vector of worker threads
    RingBufferTaskQueue<TaskGraphNode*> m_task_queue;    //!< concurrent task queue


    std::atomic_uint8_t m_num_threads_finished;    //!< number of threads finished their tasks
    std::atomic_bool m_stop_signal;    //!< acquires 'true' when the sink is to be stopped

    /*!< equals 0 if all tasks have been completed without errors. Acquires a non-zero value otherwise. 
     The value acquired in the latter case contains a pointer to the task graph node was the source of the error.
    */
    std::atomic_uint64_t m_error_watchdog;
};

}

#endif    // LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
