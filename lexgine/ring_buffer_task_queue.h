#ifndef LEXGINE_CORE_CONCURRENCY_RING_BUFFER_TASK_QUEUE_H

#include "ring_buffer_allocator.h"
#include "lock_free_queue.h"

namespace lexgine {namespace core {namespace concurrency{

class AbstractTask;    // forward declaration of a concurrent task

//! Lock-free queue based on ring buffer of fixed capacity
class RingBufferTaskQueue final
{
public:
    RingBufferTaskQueue(size_t ring_buffer_capacity = 128U);
    RingBufferTaskQueue(RingBufferTaskQueue const&) = delete;
    RingBufferTaskQueue& operator=(RingBufferTaskQueue const&) = delete;

    void enqueueTask(AbstractTask const* p_task);    //! adds new task into the queue

    misc::Optional<AbstractTask const*> dequeueTask();    //! removes task from queue

    //! Forces physical deallocation of all memory buffers marked for removal on the calling thread
    void clearCache();

    //! Shutdowns the queue making it unusable on the calling thread. This function must be called when the thread is about to exit at the latest
    void shutdown();

    //! Returns 'true' if the queue is empty; returns 'false' otherwise
    bool isEmpty() const;

private:
    LockFreeQueue<AbstractTask const*, RingBufferAllocator> m_lock_free_queue;

};

}}}

#define LEXGINE_CORE_CONCURRENCY_RING_BUFFER_TASK_QUEUE_H
#endif