#include "ring_buffer_task_queue.h"

using namespace lexgine::core::concurrency;

RingBufferTaskQueue::RingBufferTaskQueue(size_t ring_buffer_capacity):
    m_lock_free_queue{ ring_buffer_capacity }
{
}

void RingBufferTaskQueue::enqueueTask(AbstractTask* p_task)
{
    m_lock_free_queue.enqueue(p_task);
}

lexgine::core::misc::Optional<AbstractTask*> lexgine::core::concurrency::RingBufferTaskQueue::dequeueTask()
{
    return m_lock_free_queue.dequeue();
}

void RingBufferTaskQueue::clearCache()
{
    m_lock_free_queue.clearCache();
}

void RingBufferTaskQueue::shutdown()
{
    m_lock_free_queue.shutdown();
}

bool RingBufferTaskQueue::isEmpty() const
{
    return m_lock_free_queue.isEmpty();
}
