#ifndef LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H

#include "optional.h"

#include <atomic>

namespace lexgine {namespace core {namespace concurrency {

/*! Implements generic lock-free queue supporting single enqueuing thread and multiple dequeuing threads.
 The implementation is based on the paper "Simple, Fast, and Practical Non-blocking and Blocking Concurrent Queue Algorithms" by Michael, M.M., and Scott, M.L.
*/
template<typename T>
class LockFreeQueue
{
private:
    struct Node;

    //! Encapsulates pointer together with access counter
    struct NodePointer
    {
        std::atomic<Node*> ptr;    //!< actual pointer to the following node
        std::atomic<int32_t>* count;    //!< reference counter for the pointer. This is needed to avoid the ABA and the memory reclamation problems. The special value of -1 means that the pointer was already removed

        NodePointer()
        {
            std::atomic_init(&ptr, NULL);


        }

        NodePointer(Node* ptr)
        {
            std::atomic_init(&this->ptr, ptr);


        }

        NodePointer(NodePointer const& other)
        {
            Node* other_pointer = other.ptr.load(memory_order_acquire);
            ptr.store(&other_pointer, memory_order_release);

            int32_t other_pointer_count = other.count.load(memory_order_acquire);
        }

        ~NodePointer()
        {

        }
    };

    struct Node
    {
        T value;    //!< actual value of the node
        NodePointer next;    //!< "combined pointer" to the following node
    };


public:
    using value_type = T;

    LockFreeQueue()
    {

    }

    ~LockFreeQueue()
    {

    }

    //! adds new node with the given value into the queue
    void enqueue(T const& value)
    {

    }


    /*! removes the last node from the queue and returns this node wrapped into the Optional return type.
     If the queue was empty the returned Optional will have invalid value
    */
    misc::Optional<T> dequeue()
    {

    }


    /*! retrieves the size of the queue. Note that the returned value is only an estimated size of the queue. The actual size can be
     different since the nodes might have been removed or added into the queue between the moment of invocation of this function and
     the moment the function returns.
    */
    uint32_t size() const { return m_queue_element_counter.load(); }


private:
    std::atomic<NodePointer> m_head;    //!< head of the queue
    std::atomic<NodePointer> m_tail;    //!< tail of the queue
    atomic_uint32_t m_queue_element_counter;    //!< number of elements in the queue
};

}}}

#define LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H
#endif