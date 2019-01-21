#ifndef LEXGINE_CORE_CONCURRENCY_RING_BUFFER_TASK_QUEUE_H
#define LEXGINE_CORE_CONCURRENCY_RING_BUFFER_TASK_QUEUE_H

#include <algorithm>

#include "lexgine/core/ring_buffer_allocator.h"
#include "lock_free_queue.h"

namespace lexgine::core::concurrency{

//! Lock-free queue based on ring buffer of fixed capacity
template<typename TaskType>
class RingBufferTaskQueue final
{
public:
    using task_type = TaskType;

    RingBufferTaskQueue(uint16_t num_consumers,
        size_t ring_buffer_capacity = 64U,
        uint32_t garbage_collection_threshold = 10U) :
        m_lock_free_queue{ (num_consumers + 1) * (std::max)(static_cast<size_t>(garbage_collection_threshold), ring_buffer_capacity) }
    {
        m_lock_free_queue.setGarbageCollectionThreshold(garbage_collection_threshold);
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

}


#endif