#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task_graph.h"
#include "../class_names.h"

#include <vector>

namespace lexgine {namespace core {namespace concurrency {

//! Implements task scheduling based on provided task graph
class TaskSink : public NamedEntity<class_names::TaskSink>
{
public:
    TaskSink(
        TaskGraph const& source_task_graph, 
        std::vector<std::ostream*> const& worker_thread_logging_streams, 
        uint16_t max_frames_to_queue = 16U, 
        std::string const& debug_name = "");
    ~TaskSink();

    void run() noexcept(false);    //! begins execution of the task sink
    void dispatchExitSignal();    //! directs the sink to stop dispatching new tasks into the queue and exit the main loop as soon as the queue gets empty

private:
    void dispatch(uint8_t worker_id, std::ostream* logging_stream, int8_t logging_time_zone, bool logging_dts);    //! function looped by worker threads


    using worker_thread_context = std::pair<std::thread, std::ostream*>;
    std::list<worker_thread_context> m_workers_list;    //!< list of work threads
    std::vector<TaskGraph> m_task_graphs;    //!< task graphs that can be executed concurrently
    std::vector<std::atomic_bool> m_task_graph_execution_busy_vector;    //!< vector of weak locks that identify occupied concurrent frame execution buckets
    RingBufferTaskQueue<TaskGraphNode*> m_task_queue;    //!< concurrent task queue

    std::atomic_bool m_exit_signal;    //!< becomes 'true' when the sink encounters an exit task, which has been successfully executed
    std::atomic_uint16_t m_exit_level;    //!< counts number of uncompleted frames, this is needed to control stopping of the main thread only when all slave threads have completed their jobs
    
    /*!< equals 0 if all tasks have been executing without errors. Acquires non-zero value otherwise. 
     The acquired value stores pointer to the task graph node that yielded the error.
    */
    std::atomic_uint64_t m_error_watchdog;

    class TaskGraphEndExecutionGuard;

    //!< special task that is attached as dependency to all tasks of the given task graph and is needed to signalize completion of concurrent frame execution
    std::unique_ptr<TaskGraphEndExecutionGuard> m_task_graph_end_execution_guarding_task;
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#endif
