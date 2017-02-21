#ifndef LEXGINE_CORE_CONCURRENCY_RING_BUFFER_TASK_QUEUE_H

#include "ring_buffer_allocator.h"
#include "lock_free_queue.h"

namespace lexgine {namespace core {namespace concurrency{

//! Lock-free queue based on ring buffer of fixed capacity
template<typename TaskType>
class RingBufferTaskQueue final
{
public:
    using task_type = TaskType;

    RingBufferTaskQueue(size_t ring_buffer_capacity = 128U) :
        m_lock_free_queue{ ring_buffer_capacity }
    {

    }

    RingBufferTaskQueue(RingBufferTaskQueue const&) = delete;
    RingBufferTaskQueue& operator=(RingBufferTaskQueue const&) = delete;

    //! adds new task into the queue
    void enqueueTask(TaskType p_task)
    {
        m_lock_free_queue.enqueue(p_task);
    }

    //! removes task from queue
    misc::Optional<TaskType> dequeueTask()
    {
        return m_lock_free_queue.dequeue();
    }

    //! Forces physical deallocation of all memory buffers marked for removal on the calling thread
    void clearCache()
    {
        m_lock_free_queue.clearCache();
    }

    //! Shutdowns the queue making it unusable on the calling thread. This function must be called when the thread is about to exit at the latest
    void shutdown()
    {
        m_lock_free_queue.shutdown();
    }

    //! Returns 'true' if the queue is empty; returns 'false' otherwise
    bool isEmpty() const
    {
        return m_lock_free_queue.isEmpty();
    }

private:
    LockFreeQueue<TaskType, RingBufferAllocator> m_lock_free_queue;

};

}}}

#define LEXGINE_CORE_CONCURRENCY_RING_BUFFER_TASK_QUEUE_H
#endif