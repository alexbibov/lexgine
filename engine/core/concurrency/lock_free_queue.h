#ifndef LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H
#define LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H


#include <cassert>
#include <array>

#include "engine/core/misc/optional.h"
#include "engine/core/ring_buffer_allocator.h"


namespace lexgine::core::concurrency {

/*! Implements generic lock-free queue supporting multiple producers and consumers.
 The implementation is based on the ideas from paper "Simple, Fast, and Practical Non-blocking and Blocking Concurrent Queue Algorithms" by Michael, M.M., and Scott, M.L.
*/
template<typename T>
class LockFreeQueue final
{
public:
    template<typename ... allocator_construction_params>
    LockFreeQueue(allocator_construction_params... args)
        : m_allocator{ args... }
#ifdef _DEBUG
        , m_num_elements_enqueued{ 0U }
        , m_num_elements_dequeued{ 0U }
#endif
    {
        auto p_dummy_node = m_allocator.allocate();
        std::atomic_init(&p_dummy_node->next, 0xffffffff);    // next = 0xffffffff encodes invalid pointer
        std::atomic_init(&m_head, 0x0000000100000000 | static_cast<allocator_type::address_type::value_type>(p_dummy_node));
        std::atomic_init(&m_tail, 0x0000000100000000 | static_cast<allocator_type::address_type::value_type>(p_dummy_node));
    }

    LockFreeQueue(LockFreeQueue const&) = delete;
    LockFreeQueue& operator=(LockFreeQueue const&) = delete;

    //! Inserts new value into the queue
    void enqueue(T const& value)
    {
        typename allocator_type::address_type new_node_ptr{ &m_allocator };
        
        new_node_ptr = m_allocator.allocate();
        new_node_ptr->data = value;
        std::atomic_init(&new_node_ptr->next, 0xffffffff);    // next = 0xffffffff encodes invalid pointer
        uint64_t tail{};
        while (true)
        {
            tail = m_tail.load(std::memory_order::memory_order_acquire);
            typename allocator_type::address_type tail_ptr{ tail & 0x00000000ffffffff, &m_allocator };
            uint64_t next = tail_ptr->next.load(std::memory_order::memory_order_acquire);
            uint64_t next_ptr = next & 0x00000000ffffffff;

            if (tail == m_tail.load(std::memory_order::memory_order_consume))    // check if p_tail is still related to the queue...
            {
                if (next_ptr == 0xffffffff)
                {
                    // the tail node still points at the tail of the queue, hence we can try to add the new node into the queue
                    if (tail_ptr->next.compare_exchange_strong(
                        next, 
                        0x0000000100000000 | static_cast<uint32_t>(new_node_ptr), 
                        std::memory_order::memory_order_acq_rel,
                        std::memory_order::acquire
                    ))
                    {
                        break;    // enqueue has succeeded
                    }
                }
                else
                {
                    // the tail node is not actually the tail any longer, attempt to move it forward...
                    m_tail.compare_exchange_strong(
                        tail, 
                        ((tail & 0xffffffff00000000) + 0x0000000100000000) | next_ptr,
                        std::memory_order::memory_order_acq_rel, 
                        std::memory_order_relaxed)
                        ;
                }
            }
        }

        // Enqueue is done: move the tail node pointer forward
        m_tail.compare_exchange_strong(
            tail, 
            ((tail & 0xffffffff00000000) + 0x0000000100000000) | static_cast<uint32_t>(new_node_ptr), 
            std::memory_order::memory_order_acq_rel,
            std::memory_order::acquire
        );

#ifdef _DEBUG
        ++m_num_elements_enqueued;
#endif

        return;
    }


    /*! Retrieves the last entry from the queue. If the queue is empty returns invalid Optional object.
     Otherwise, returns an Optional<T> wrapping the value contained in the retrieved node.
    */
    misc::Optional<T> dequeue()
    {
        while (true)
        {
            uint64_t head = m_head.load(std::memory_order::memory_order_acquire);
            uint64_t tail = m_tail.load(std::memory_order::memory_order_acquire);
            typename allocator_type::address_type head_ptr { head & 0x00000000ffffffff, &m_allocator };
            typename allocator_type::address_type tail_ptr { tail & 0x00000000ffffffff, &m_allocator };

            uint64_t next = head_ptr->next.load(std::memory_order::memory_order_acquire);
            typename allocator_type::address_type next_ptr { next & 0x00000000ffffffff, &m_allocator };

            if (head == m_head.load(std::memory_order::memory_order_consume))    // verify if head is consistent
            {
                // Check if the queue is empty or tail needs to be advanced in order to avoid spuriously assuming that the queue was exhausted
                if (head_ptr == tail_ptr)
                {
                    // the queue is empty or the tail has fallen behind
                    if (next_ptr == typename allocator_type::address_type{ 0xffffffff, &m_allocator })
                    {
                        return misc::Optional<T>{};    // the queue is empty. Return invalid value container.
                    }

                    // We end up here if the queue is not empty, but the head and the tail pointers refer to the same node.
                    // In this case we attempt to move the tail pointer forward
                    m_tail.compare_exchange_strong(
                        tail, 
                        static_cast<allocator_type::address_type::value_type>(next_ptr) | ((tail & 0xffffffff00000000) + 0x0000000100000000), 
                        std::memory_order::memory_order_acq_rel
                    );
                }
                else
                {
                    T data = next_ptr->data;
                    if (m_head.compare_exchange_strong(
                        head,
                        static_cast<allocator_type::address_type::value_type>(next_ptr) | ((head & 0xffffffff00000000) + 0x0000000100000000),
                        std::memory_order::memory_order_acq_rel,
                        std::memory_order_acquire
                    )) 
                    {
#ifdef _DEBUG
                        ++m_num_elements_dequeued;
#endif
                        misc::Optional<T> rv{ data };
                        m_allocator.free(head_ptr);

                        return rv;
                    }
                }

            }
        }
    }


    //! Forces physical deallocation of all memory buffers marked for removal on the calling thread
    void clearCache()
    {
        // m_hp_pool.flush();
    }


    //! Shutdowns the queue for the calling thread. The thread will not be able to use the queue after this function is called and it must be called when the thread is about to exit at the latest
    void shutdown()
    {
        // m_hp_pool.cleanup();
    }


    //! Returns 'true' if the queue is empty; returns 'false' otherwise
    bool isEmpty() const
    {
        return m_head.load(std::memory_order::memory_order_consume) == m_tail.load(std::memory_order::memory_order_consume);
    }

    /*void setGarbageCollectionThreshold(uint32_t threshold)
    {
        m_hp_pool.setGCThreshold(threshold);
    }*/


    ~LockFreeQueue()
    {
        uint64_t last_node = m_head.load(std::memory_order::memory_order_consume);
        uint64_t tail = m_tail.load(std::memory_order::memory_order_acquire);

        typename allocator_type::address_type last_node_ptr{ last_node & 0x00000000ffffffff, &m_allocator };
        typename allocator_type::address_type tail_ptr{ tail & 0x00000000ffffffff, &m_allocator };

        assert(last_node_ptr == tail_ptr);    // the queue must be empty when getting destructed

#ifdef _DEBUG
        assert(m_num_elements_enqueued == m_num_elements_dequeued);
#endif

        m_allocator.free(last_node_ptr);
    }


private:
    //! Describes single node of the queue
    struct Node
    {
        T data;    //!< the data contained in the queue node
        std::atomic_uint64_t next;    //!< atomic pointer to the next member of the queue
    };

    using allocator_type = RingBufferAllocator<Node>;

    std::atomic_uint64_t m_head, m_tail;    //!< head and tail of the underlying queue data structure
    allocator_type m_allocator;    //!< allocator used by the queue

#ifdef _DEBUG
    std::atomic_uint32_t m_num_elements_enqueued;    //!< total number of elements ever added into the queue
    std::atomic_uint32_t m_num_elements_dequeued;    //!< total number of elements ever removed from the queue
#endif

    std::array<uint8_t, 64> m_padding; //!< padding to avoid false sharing
};

} // namespace lexgine::core::concurrency

#endif