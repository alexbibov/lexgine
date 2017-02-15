#include "task_sink.h"

using namespace lexgine::core::concurrency;

TaskSink::TaskSink(TaskGraph const& source_task_graph):
    m_task_list{ TaskGraphAttorney<TaskSink>::getTaskList(source_task_graph) }
{

}
