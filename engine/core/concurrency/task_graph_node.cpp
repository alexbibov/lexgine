#include "task_graph_node.h"
#include "abstract_task.h"

#include <cassert>

using namespace lexgine::core::concurrency;


namespace {
static uint64_t id_counter = 0U;
}

TaskGraphNode::TaskGraphNode(AbstractTask& task) :
    m_id{ ++id_counter },
    m_contained_task{ &task },
    m_is_completed{ false },
    m_is_scheduled{ false }
{

}

TaskGraphNode::TaskGraphNode(TaskGraphNode const& other) :
    m_id{ other.m_id },
    m_contained_task{ other.m_contained_task },
    m_is_completed{ false },
    m_is_scheduled{ false }
{

}

TaskGraphNode::TaskGraphNode(TaskGraphNode&& other) :
    m_id{ other.m_id },
    m_contained_task{ other.m_contained_task },
    m_is_completed{ other.m_is_completed.load(std::memory_order_acquire) },
    m_is_scheduled{ other.m_is_scheduled.load(std::memory_order_acquire) },
    m_dependencies{ std::move(other.m_dependencies) },
    m_dependents{ std::move(other.m_dependents) }
{

}

bool lexgine::core::concurrency::TaskGraphNode::operator==(TaskGraphNode const& other) const
{
    return m_id == other.m_id;
}

bool TaskGraphNode::execute(uint8_t worker_id)
{
    bool rv = m_contained_task->execute(worker_id, m_user_data);
    m_is_completed.store(rv && !m_contained_task->getErrorState(), std::memory_order_release);
    return rv;
}

bool TaskGraphNode::isCompleted() const
{
    return m_is_completed.load(std::memory_order_acquire);
}

void TaskGraphNode::schedule(RingBufferTaskQueue<TaskGraphNode*>& queue)
{
    if(!m_is_scheduled.load(std::memory_order_acquire))
    {
        m_is_scheduled.store(true, std::memory_order_release);
        queue.enqueueTask(this);
    }
}

bool TaskGraphNode::isReadyToLaunch() const
{
    for (auto node : m_dependencies)
    {
        if (!node->isCompleted()) return false;
    }

    return true;
}

bool TaskGraphNode::isScheduled() const
{
    return m_is_scheduled.load(std::memory_order_acquire);
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

TaskGraphNode TaskGraphNode::clone() const
{
    return TaskGraphNode{ *this };
}

uint64_t TaskGraphNode::getId() const
{
    return m_id;
}

TaskGraphNode::set_of_nodes TaskGraphNode::getDependencies() const
{
    return m_dependencies;
}

TaskGraphNode::set_of_nodes TaskGraphNode::getDependents() const
{
    return m_dependents;
}

void TaskGraphNode::resetExecutionStatus()
{
    m_is_completed.store(false, std::memory_order_release);
    m_is_scheduled.store(false, std::memory_order_release);
}

AbstractTask* TaskGraphNode::task() const
{
    return m_contained_task;
}

void TaskGraphNode::setUserData(uint64_t user_data)
{
    m_user_data = user_data;
}

uint64_t TaskGraphNode::getUserData() const
{
    return m_user_data;
}

TaskGraphRootNode::TaskGraphRootNode(AbstractTask& task) :
    TaskGraphNode{ task }
{
}

bool TaskGraphRootNode::addDependency(TaskGraphNode& task)
{
    assert(false);
    return false;
}
