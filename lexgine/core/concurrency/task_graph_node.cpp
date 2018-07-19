#include "task_graph_node.h"
#include "abstract_task.h"

#include <cassert>

using namespace lexgine::core::concurrency;


namespace {
static uint64_t id_counter = 0U;
}

TaskGraphNode::TaskGraphNode(AbstractTask& task, uint64_t id) :
    m_id{ id },
    m_contained_task{ &task },
    m_is_completed{ false },
    m_is_scheduled{ false },
    m_frame_index{ 0U }
{

}

TaskGraphNode::TaskGraphNode(AbstractTask& task) :
    TaskGraphNode{ task, ++id_counter }
{

}

bool lexgine::core::concurrency::TaskGraphNode::operator==(TaskGraphNode const& other) const
{
    return m_id == other.m_id;
}

bool TaskGraphNode::execute(uint8_t worker_id)
{
    bool rv = m_contained_task->execute(worker_id, m_frame_index);
    m_is_completed.store(rv && !m_contained_task->getErrorState(), std::memory_order_release);
    return rv;
}

bool TaskGraphNode::isCompleted() const
{
    return m_is_completed.load(std::memory_order_acquire);
}

void TaskGraphNode::schedule(RingBufferTaskQueue<TaskGraphNode*>& queue)
{
    if(!m_is_scheduled)
    {
        queue.enqueueTask(this);
        m_is_scheduled = true;
    }
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

bool TaskGraphNode::addDependent(TaskGraphNode& task)
{
    m_dependents.insert(&task);
    return task.m_dependencies.insert(this).second;
}

bool TaskGraphNode::addDependency(TaskGraphNode& task)
{
    m_dependencies.insert(&task);
    return task.m_dependents.insert(this).second;
}

uint16_t TaskGraphNode::frameIndex() const
{
    return m_frame_index;
}

void TaskGraphNode::forceUndone()
{
    m_is_completed.store(false, std::memory_order_release);
    m_is_scheduled = false;
}
