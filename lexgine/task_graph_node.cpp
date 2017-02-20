#include "task_graph_node.h"
#include "abstract_task.h"

#include <cassert>

using namespace lexgine::core::concurrency;


namespace {
static uint32_t id_counter = 0U;
}


TaskGraphNode::TaskGraphNode(AbstractTask& task) :
    m_id{ ++id_counter },
    m_contained_task{ &task },
    m_is_completed{ false },
    m_visit_flag{ false },
    m_frame_index{ 0U }
{

}

TaskGraphNode::TaskGraphNode(AbstractTask & task, uint32_t id):
    m_id{ id },
    m_contained_task{ &task },
    m_is_completed{ false },
    m_visit_flag{ false },
    m_frame_index{ 0U }
{
}

bool TaskGraphNode::execute(uint8_t worker_id)
{
    return m_contained_task->execute(worker_id, m_frame_index);
}

bool TaskGraphNode::isCompleted() const
{
    return m_is_completed;
}

bool TaskGraphNode::isReadyToLaunch() const
{
    for (auto node : m_dependencies)
    {
        if (!node->isCompleted())
            return false;
    }

    return true;
}

void TaskGraphNode::addDependent(TaskGraphNode& task)
{
    m_dependents.push_back(&task);
    task.m_dependencies.push_back(this);
}

