#ifndef LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H

#include "optional.h"
#include "hazard_pointer_pool.h"
#include <cassert>


namespace lexgine {namespace core {namespace concurrency {

/*! Implements generic lock-free queue supporting multiple producers and consumers.
 The implementation is based on the ideas from paper "Simple, Fast, and Practical Non-blocking and Blocking Concurrent Queue Algorithms" by Michael, M.M., and Scott, M.L.
*/
template<typename T>
class LockFreeQueue
{
public:
    LockFreeQueue() :
#ifdef _DEBUG
        m_num_elements_enqueued{ 0U }
        , m_num_elements_dequeued{ 0U }
#endif
    {
        Node* p_dummy_node = static_cast<Node*>(malloc(sizeof(Node)));
        std::atomic_init(&p_dummy_node->next, nullptr);
        std::atomic_init(&m_head, p_dummy_node);
        std::atomic_init(&m_tail, p_dummy_node);
    }


    //! Inserts new value into the queue
    void enqueue(T const& value)
    {
        Node* p_new_node = static_cast<Node*>(malloc(sizeof(Node)));
        p_new_node->data = value;
        std::atomic_init(&p_new_node->next, nullptr);

        while (true)
        {
            HazardPointerPool::HazardPointerRecord hp_tail = hp_pool.acquire(m_tail.load(std::memory_order::memory_order_acquire));

            Node* p_tail = static_cast<Node*>(hp_tail.get());
            Node* p_next = p_tail->next.load(std::memory_order::memory_order_consume);


            if (p_tail == m_tail.load(std::memory_order::memory_order_consume))    // check if p_tail is still related to the queue...
            {
                if (p_next != nullptr)
                {
                    // the tail node is not actually the tail any longer, attempt to move it forward...
                    m_tail.compare_exchange_weak(p_tail, p_next, std::memory_order::memory_order_acq_rel);
                }
                else
                {
                    // the tail node still points at the tail of the queue, hence we can try to add the new node into the queue
                    if (p_tail->next.compare_exchange_weak(p_next, p_new_node, std::memory_order::memory_order_acq_rel))
                    {
                        // if we were successful attempt to move the tail node pointer forward
                        m_tail.compare_exchange_weak(p_tail, p_new_node, std::memory_order::memory_order_acq_rel);

                        ++m_num_elements_enqueued;

                        return;
                    }
                }
            }
        }
    }


    /*! Retrieves the last entry from the queue. If the queue is empty returns invalid Optional object.
     Otherwise, returns an Optional<T> wrapping the value contained in the retrieved node.
    */
    misc::Optional<T> dequeue()
    {
        while (true)
        {
            HazardPointerPool::HazardPointerRecord hp_head = hp_pool.acquire(m_head.load(std::memory_order::memory_order_acquire));
            Node* p_head = static_cast<Node*>(hp_head.get());

            HazardPointerPool::HazardPointerRecord hp_head_next{};
            Node* p_head_next{ nullptr };

            HazardPointerPool::HazardPointerRecord hp_tail = hp_pool.acquire(m_tail.load(std::memory_order::memory_order_acquire));
            Node* p_tail = static_cast<Node*>(hp_tail.get());

            if (p_head == m_head.load(std::memory_order::memory_order_consume))    // check if p_head is still related to the queue
            {
                // Now we can be sure that we can access the node that follows the head node...
                hp_head_next = hp_pool.acquire(p_head->next.load(std::memory_order::memory_order_acquire));
                p_head_next = static_cast<Node*>(hp_head_next.get());


                if (p_head == p_tail)
                {
                    if (p_head_next == nullptr)
                    {
                        return misc::Optional<T>{};    // the queue is empty. Return invalid value container.
                    }

                    // We end up here if the queue is not empty, but the head and the tail pointers refer to the same node.
                    // In this case we attempt to move the tail pointer forward
                    m_tail.compare_exchange_weak(p_tail, p_head_next, std::memory_order::memory_order_acq_rel);
                }
                else
                {
                    // the queue was definitely not empty

                    if (p_head == m_head.load(std::memory_order::memory_order_consume))
                    {
                        misc::Optional<T> rv = p_head_next->data;

                        if (m_head.compare_exchange_weak(p_head, p_head_next, std::memory_order::memory_order_acq_rel))
                        {
                            hp_pool.retire(hp_head);

                            ++m_num_elements_dequeued;

                            return rv;
                        }
                    }

                }

            }
        }
    }


    //! Forces physical deallocation of all memory buffers marked for removal on the calling thread
    void clearCache()
    {
        hp_pool.flush();
    }


    //! Returns 'true' if the queue is empty; returns 'false' otherwise
    bool isEmpty() const
    {
        return m_head.load(std::memory_order::memory_order_consume) == m_tail.load(std::memory_order::memory_order_consume);
    }


    ~LockFreeQueue()
    {
        Node* p_last_node_to_destruct = m_head.load(std::memory_order::memory_order_consume);
        assert(p_last_node_to_destruct == m_tail.load(std::memory_order::memory_order_consume));    // the queue must be empty when getting destructed

#ifdef _DEBUG
        assert(m_num_elements_enqueued == m_num_elements_dequeued);
#endif
        free(p_last_node_to_destruct);
    }


private:
    //! Describes single node of the queue
    struct Node
    {
        T data;    //!< the data contained in the queue node
        std::atomic<Node*> next;    //!< atomic pointer to the next member of the queue
    };

    std::atomic<Node*> m_head, m_tail;    //!< head and tail of the underlying queue data structure
    HazardPointerPool hp_pool;    //!< pool of hazard pointer employed for safe memory reclamation

#ifdef _DEBUG
    std::atomic_uint32_t m_num_elements_enqueued;    //!< total number of elements ever added into the queue
    std::atomic_uint32_t m_num_elements_dequeued;    //!< total number of elements ever removed from the queue
#endif
};

}}}

#define LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H
#endif