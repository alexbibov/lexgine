#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task_graph.h"
#include "ring_buffer_task_queue.h"

#include <vector>


namespace lexgine {namespace core {namespace concurrency {

//! Implements task scheduling based on provided task graph
class TaskSink : public NamedEntity<class_names::TaskSink>
{
public:
    TaskSink(std::string const& debug_name, TaskGraph const& source_task_graph, std::vector<std::ostream*> const& worker_thread_logging_streams);
    TaskSink(TaskGraph const& source_task_graph, std::vector<std::ostream*> const& worker_thread_logging_streams);

    void run(bool loop_tasks = true);    //! runs the task sink

private:
    void dispatch(uint8_t worker_id, std::ostream* logging_stream, int8_t logging_time_zone, bool logging_dts);    //!< function looped by worker threads

    using worker_thread_context = std::pair<std::thread, std::ostream*>;
    std::list<AbstractTask*> const& m_task_list;    //!< reference to the list of tasks provided by the task graph
    std::list<worker_thread_context> m_workers_list;    //!< list of work threads

    RingBufferTaskQueue m_task_queue;    //!< concurrent task queue
    bool m_exit_signal;    //!< becomes 'true' when the sink encounters an exit task, which has been successfully executed
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#endif
