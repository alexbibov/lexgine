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
        Node* ptr;    //!< actual pointer to the following node
        uint32_t counter;    //!< number of times the pointer has been updated after creation (needed to circumvent the ABA-problem)
    };

    struct Node
    {
        T value;    //!< actual value of the node
        std::atomic<NodePointer> next;    //!< "combined pointer" to the following node
    };


public:
    using value_type = T;

    LockFreeQueue()
    {
        Node* dummy_node = new Node{};
        std::atomic_init(&dummy_node->next, NodePointer{ nullptr, 0U });

        std::atomic_init(&m_head, NodePointer{ dummy_node, 0U });
        std::atomic_init(&m_tail, NodePointer{ dummy_node, 0U });
        std::atomic_init(&m_queue_element_counter, 0U);
    }

    ~LockFreeQueue()
    {

    }

    //! adds new node with the given value into the queue
    void enqueue(T const& value)
    {
        Node* p_node = new Node{};
        p_node->value = value;
        std::atomic_init(&p_node->next, NodePointer{ nullptr, 0U });

        NodePointer volatile tail_estimate;
        NodePointer volatile next_estimate;
        while (true)
        {
            tail_estimate = m_tail.load();
            next_estimate = tail_estimate.ptr->next.load();

            // Check if the loaded tail corresponds to the current tail value
            NodePointer volatile aux = m_tail.load();
            if (std::memcmp(&tail_estimate, &aux, sizeof(NodePointer)) == 0)
            {
                // Is the "next" node actually a NULL-pointer?
                if (next_estimate.ptr == nullptr)
                {
                    // Attempt to link the new node to the queue
                    NodePointer new_node_pointer{ p_node, next_estimate.counter + 1 };
                    if(tail_estimate.ptr->next.compare_exchange_weak(&next_estimate, &new_node_pointer))
                        break;
                }
                else
                {
                    // Tail does not point to the last node in the queue. Attempt to advance it.
                    NodePointer new_tail_pointer{ next_estimate.ptr, tail_estimate.counter + 1 };
                    m_tail.compare_exchange_weak(&tail_estimate, &new_tail_pointer);
                }
            }
        }

        // Set the tail to point to the newly inserted node
        NodePointer new_tail_value{ p_node, tail_estimate.count + 1 };
        m_tail.compare_exchange_weak(&tail_estimate, &new_tail_value);

        ++m_queue_element_counter;
    }


    /*! removes the last node from the queue and returns this node wrapped into the Optional return type.
     If the queue was empty the returned Optional will have invalid value
    */
    misc::Optional<T> dequeue()
    {
        NodePointer volatile head_estimate;
        NodePointer volatile tail_estimate;
        NodePointer volatile next_estimate;
        while (true)
        {
            head_estimate = m_head.load();
            tail_estimate = m_tail.load();
            next_estimate = head_estimate.ptr->next.load();

            // Check if the head value is consistent
            NodePointer volatile aux = m_head.load();
            if (std::memcmp(&head_estimate, &aux, sizeof(NodePointer)) == 0)
            {
                if (head_estimate.ptr == tail_estimate.ptr)    // the queue is either empty or its tail is falling behind
                {
                    if (next_estimate.ptr == nullptr)
                    {
                        // the queue is empty, return invalid value
                        return misc::Optional<T>{};
                    }
                    // attempt to advance the tail
                    NodePointer new_tail_value{ next_estimate.ptr, tail_estimate.counter + 1 };
                    m_tail.compare_exchange_weak(&tail_estimate, &new_tail_value);
                }
                else
                {
                    T dequeued_value = next_estimate.ptr->value;
                }
            }
        }

        --m_queue_element_counter;
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