#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task_graph.h"
#include "ring_buffer_task_queue.h"
#include "class_names.h"

#include <vector>


namespace lexgine {namespace core {namespace concurrency {

//! Implements task scheduling based on provided task graph
class TaskSink : public NamedEntity<class_names::TaskSink>
{
public:
    TaskSink(TaskGraph const& source_task_graph, std::vector<std::ostream*> const& worker_thread_logging_streams, std::string const& debug_name = "");

    /*!
     Begins execution of the task sink. Input parameter determines how many task graphs
     can be executed concurrently.
    */
    void run(uint16_t max_frames_queued = 16U);

private:
    void dispatch(uint8_t worker_id, std::ostream* logging_stream, int8_t logging_time_zone, bool logging_dts);    //! function looped by worker threads


    using worker_thread_context = std::pair<std::thread, std::ostream*>;
    std::list<worker_thread_context> m_workers_list;    //!< list of work threads


    RingBufferAllocator<TaskGraph> m_task_graph_merry_go_round;    //!< ring buffer holding concurrent frame execution
    RingBufferTaskQueue m_task_queue;    //!< concurrent task queue
    bool m_exit_signal;    //!< becomes 'true' when the sink encounters an exit task, which has been successfully executed
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#endif
