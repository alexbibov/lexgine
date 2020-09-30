#ifndef LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H

#include "../misc/optional.h"
#include "hazard_pointer_pool.h"
#include "../default_allocator.h"

#include <cassert>


namespace lexgine {namespace core {namespace concurrency {

/*! Implements generic lock-free queue supporting multiple producers and consumers.
 The implementation is based on the ideas from paper "Simple, Fast, and Practical Non-blocking and Blocking Concurrent Queue Algorithms" by Michael, M.M., and Scott, M.L.
*/
template<typename T, template<typename> typename AllocatorTemplate = DefaultAllocator>
class LockFreeQueue
{
public:
    template<typename ... allocator_construction_params>
    LockFreeQueue(allocator_construction_params... args)
        : m_allocator{ args... }
        , m_hp_pool{ m_allocator }
#ifdef _DEBUG
        , m_num_elements_enqueued{ 0U }
        , m_num_elements_dequeued{ 0U }
#endif
    {
        auto p_dummy_node = m_allocator.allocate();
        std::atomic_init(&p_dummy_node->next, 0U);
        std::atomic_init(&m_head, static_cast<size_t>(p_dummy_node));
        std::atomic_init(&m_tail, static_cast<size_t>(p_dummy_node));
    }

    LockFreeQueue(LockFreeQueue const&) = delete;
    LockFreeQueue& operator=(LockFreeQueue const&) = delete;


    //! Inserts new value into the queue
    void enqueue(T const& value)
    {
        allocator_type::address_type p_new_node = nullptr;
        
        while (!p_new_node)
        {
            p_new_node = m_allocator.allocate();
        }
        p_new_node->data = value;
        std::atomic_init(&p_new_node->next, 0U);

        while (true)
        {
            hpp_type::HazardPointerRecord hp_tail = m_hp_pool.acquire(allocator_type::address_type{ m_tail.load(std::memory_order::memory_order_acquire) });

            allocator_type::address_type p_tail = hp_tail.get();


            if (static_cast<size_t>(p_tail) == m_tail.load(std::memory_order::memory_order_consume))    // check if p_tail is still related to the queue...
            {
                size_t p_next = p_tail->next.load(std::memory_order::memory_order_consume);    // now we can safely access the data through the pointer as it was still related to the queue whilst being hazardous

                if (p_next != 0U)
                {
                    // the tail node is not actually the tail any longer, attempt to move it forward...
                    size_t p_tail_addr = static_cast<size_t>(p_tail);
                    m_tail.compare_exchange_strong(p_tail_addr, p_next, std::memory_order::memory_order_acq_rel);
                }
                else
                {
                    // the tail node still points at the tail of the queue, hence we can try to add the new node into the queue
                    if (p_tail->next.compare_exchange_strong(p_next, static_cast<size_t>(p_new_node), std::memory_order::memory_order_acq_rel))
                    {
                        // if we were successful attempt to move the tail node pointer forward
                        size_t p_tail_addr = static_cast<size_t>(p_tail);
                        m_tail.compare_exchange_strong(p_tail_addr, static_cast<size_t>(p_new_node), std::memory_order::memory_order_acq_rel);

#ifdef _DEBUG
                        ++m_num_elements_enqueued;
#endif

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
            hpp_type::HazardPointerRecord hp_head = m_hp_pool.acquire(allocator_type::address_type{ m_head.load(std::memory_order::memory_order_acquire) });
            allocator_type::address_type p_head = hp_head.get();

            hpp_type::HazardPointerRecord hp_head_next{};
            allocator_type::address_type p_head_next{ nullptr };

            hpp_type::HazardPointerRecord hp_tail = m_hp_pool.acquire(allocator_type::address_type{ m_tail.load(std::memory_order::memory_order_acquire) });
            allocator_type::address_type p_tail = hp_tail.get();

            if (static_cast<size_t>(p_head) == m_head.load(std::memory_order::memory_order_consume))    // check if p_head is still related to the queue
            {
                // Now we can be sure that we can access the node that follows the head node...
                hp_head_next = m_hp_pool.acquire(allocator_type::address_type{ p_head->next.load(std::memory_order::memory_order_acquire) });
                p_head_next = hp_head_next.get();

                if (p_head == p_tail)
                {
                    if (p_head_next == nullptr)
                    {
                        return misc::Optional<T>{};    // the queue is empty. Return invalid value container.
                    }

                    // We end up here if the queue is not empty, but the head and the tail pointers refer to the same node.
                    // In this case we attempt to move the tail pointer forward
                    size_t p_tail_addr = static_cast<size_t>(p_tail);
                    m_tail.compare_exchange_strong(p_tail_addr, static_cast<size_t>(p_head_next), std::memory_order::memory_order_acq_rel);
                }
                else
                {
                    // the queue was definitely not empty

                    // if p_head was relevant at this point then p_head->next was also relevant. On the other hand at the point
                    // of this check p_head->next was already hazardous and therefore cannot be deallocated thereafter. Hence, it can be used safely
                    // if the check succeeds
                    if (static_cast<size_t>(p_head) == m_head.load(std::memory_order::memory_order_consume))
                    {
                        misc::Optional<T> rv = p_head_next->data;

                        size_t p_head_addr = static_cast<size_t>(p_head);
                        if (m_head.compare_exchange_strong(static_cast<size_t>(p_head_addr), static_cast<size_t>(p_head_next), std::memory_order::memory_order_acq_rel))
                        {
                            m_hp_pool.retire(hp_head);

#ifdef _DEBUG
                            ++m_num_elements_dequeued;
#endif

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
        m_hp_pool.flush();
    }


    //! Shutdowns the queue for the calling thread. The thread will not be able to use the queue after this function is called and it must be called when the thread is about to exit at the latest
    void shutdown()
    {
        m_hp_pool.cleanup();
    }


    //! Returns 'true' if the queue is empty; returns 'false' otherwise
    bool isEmpty() const
    {
        return m_head.load(std::memory_order::memory_order_consume) == m_tail.load(std::memory_order::memory_order_consume);
    }

    void setGarbageCollectionThreshold(uint32_t threshold)
    {
        m_hp_pool.setGCThreshold(threshold);
    }


    ~LockFreeQueue()
    {
        allocator_type::address_type p_last_node_to_destruct{ m_head.load(std::memory_order::memory_order_consume) };
        assert(static_cast<size_t>(p_last_node_to_destruct) == m_tail.load(std::memory_order::memory_order_consume));    // the queue must be empty when getting destructed

#ifdef _DEBUG
        assert(m_num_elements_enqueued == m_num_elements_dequeued);
#endif
        m_allocator.free(p_last_node_to_destruct);
    }


private:
    //! Describes single node of the queue
    struct Node
    {
        T data;    //!< the data contained in the queue node
        std::atomic_size_t next;    //!< atomic pointer to the next member of the queue
    };

    using allocator_type = AllocatorTemplate<Node>;
    using hpp_type = HazardPointerPool<allocator_type>;

    std::atomic_size_t m_head, m_tail;    //!< head and tail of the underlying queue data structure

    allocator_type m_allocator;    //!< allocator used by the queue
    hpp_type m_hp_pool;    //!< pool of hazard pointer employed for safe memory reclamation

#ifdef _DEBUG
    std::atomic_uint32_t m_num_elements_enqueued;    //!< total number of elements ever added into the queue
    std::atomic_uint32_t m_num_elements_dequeued;    //!< total number of elements ever removed from the queue
#endif
};

}}}

#define LEXGINE_CORE_CONCURRENCY_LOCK_FREE_QUEUE_H
#endif