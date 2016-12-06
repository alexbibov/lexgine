#ifndef LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H

#include "optional.h"
#include "hazard_pointer_pool.h"

#include <atomic>

namespace lexgine {namespace core {namespace concurrency {

/*! Implements generic lock-free queue supporting multiple producers and consumers.
 The implementation is based on the ideas from paper "Simple, Fast, and Practical Non-blocking and Blocking Concurrent Queue Algorithms" by Michael, M.M., and Scott, M.L.
*/
template<typename T>
class LockFreeQueue
{


private:
    std::atomic<NodePointer> m_head;    //!< head of the queue
    std::atomic<NodePointer> m_tail;    //!< tail of the queue
    atomic_uint32_t m_queue_element_counter;    //!< number of elements in the queue
};

}}}

#define LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H
#endif