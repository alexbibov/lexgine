#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task_graph.h"
#include "ring_buffer_task_queue.h"


namespace lexgine {namespace core {namespace concurrency {

//! Implements task scheduling based on provided task graph
class TaskSink
{
public:
    TaskSink(TaskGraph const& source_task_graph);

    void run();    //! runs the task sink

private:

    RingBufferTaskQueue m_task_queue;    //!< concurrent task queue
    std::list<AbstractTask*> const& m_task_list;    //!< reference to the list of tasks provided by the task graph
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#endif
