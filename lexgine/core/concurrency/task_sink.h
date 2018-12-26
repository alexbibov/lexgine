#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task_graph.h"
#include "lexgine/core/class_names.h"

#include <vector>

namespace lexgine::core::concurrency {

//! Implements task scheduling based on provided task graph
class TaskSink final : public NamedEntity<class_names::TaskSink>
{
public:
    TaskSink(
        TaskGraph const& source_task_graph, 
        std::vector<std::ostream*> const& worker_thread_logging_streams,
        std::string const& debug_name = "");
    ~TaskSink();

    void run();    //! begins execution of the task sink, @param now_time is used for logging
    void dispatchExitSignal();    //! directs the sink to stop dispatching new tasks into the queue and exit the main loop as soon as the last task graph is done

private:
    void dispatch(uint8_t worker_id, std::ostream* logging_stream, int8_t logging_time_zone, bool logging_dts);    //! function looped by worker threads


    using worker_thread_context = std::pair<std::thread, std::ostream*>;
    std::list<worker_thread_context> m_workers_list;    //!< list of work threads
    TaskGraph const* m_source_task_graph_ptr;    //!< source task graph in raw representation
    std::unique_ptr<TaskGraph> m_compiled_task_graph;    //!< compiled task graph ('compiled' here means that it has been optimized for execution and cannot change any longer)
    RingBufferTaskQueue<TaskGraphNode*> m_task_queue;    //!< concurrent task queue

    std::atomic_bool m_exit_signal;    //!< becomes 'true' when the sink encounters an exit task, which has been successfully executed
    std::atomic_uint16_t m_exit_level;    //!< counts number of uncompleted frames, this is needed to control stopping of the main thread only when all slave threads have completed their jobs
    std::atomic_uint8_t m_num_threads_finished;    //!< number of threads finished their tasks

    /*!< equals 0 if all tasks have been executing without errors. Acquires non-zero value otherwise. 
     The acquired value stores pointer to the task graph node that yielded the error.
    */
    std::atomic_uint64_t m_error_watchdog;

    class TaskGraphEndExecutionGuard;

    //!< special task that is attached as dependency to all tasks of the given task graph and is needed to signalize completion of concurrent frame execution
    std::unique_ptr<TaskGraphEndExecutionGuard> m_task_graph_end_execution_guarding_task;
};

}

#endif    // LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
