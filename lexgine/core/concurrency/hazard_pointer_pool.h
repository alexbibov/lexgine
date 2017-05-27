#ifndef LEXGINE_CORE_CONCURRENCY_HAZARD_POINTER_POOL_H

#include <cstdint>
#include <memory>
#include <atomic>

#include "../allocator.h"

namespace lexgine {namespace core {namespace concurrency {

//! Pool of hazard pointers. Allows to make a given allocation hazardous, which prevents it from being freed by the concurrent threads
template<typename AllocatorInstantiation>
class HazardPointerPool
{
public:

    //! Describes hazard pointer record, which enables automatic reuse of hazard pointer entries in the cache
    class HazardPointerRecord
    {
        friend HazardPointerPool;

    public:
        HazardPointerRecord():
            m_p_hp_entry{ nullptr },
            m_ref_counter{ nullptr }
        {
        }

        HazardPointerRecord(HazardPointerRecord const& other) :
            m_p_hp_entry{ other.m_p_hp_entry },
            m_ref_counter{ other.m_ref_counter }
        {
            ++(*m_ref_counter);
        }

        HazardPointerRecord(HazardPointerRecord&& other) :
            m_p_hp_entry{ other.m_p_hp_entry },
            m_ref_counter{ other.m_ref_counter }
        {
            other.m_ref_counter = nullptr;
        }

        HazardPointerRecord& operator=(HazardPointerRecord const& other)
        {
            if (this == &other)
                return *this;

            if (m_ref_counter && !(--(*m_ref_counter)))
            {
                delete m_ref_counter;
                static_cast<HPListEntry*>(m_p_hp_entry)->is_active.store(false, std::memory_order::memory_order_release);
            }

            m_p_hp_entry = other.m_p_hp_entry;
            m_ref_counter = other.m_ref_counter;

            ++(*m_ref_counter);

            return *this;
        }

        HazardPointerRecord& operator=(HazardPointerRecord&& other)
        {
            if (this == &other)
                return *this;

            if (m_ref_counter && !(--(*m_ref_counter)))
            {
                delete m_ref_counter;
                static_cast<HPListEntry*>(m_p_hp_entry)->is_active.store(false, std::memory_order::memory_order_release);
            }

            m_p_hp_entry = other.m_p_hp_entry;
            m_ref_counter = other.m_ref_counter;
            other.m_ref_counter = nullptr;

            return *this;
        }

        ~HazardPointerRecord()
        {
            if (m_ref_counter && !(--(*m_ref_counter)))
            {
                delete m_ref_counter;
                static_cast<HPListEntry*>(m_p_hp_entry)->is_active.store(false, std::memory_order::memory_order_release);
            }
        }

        //! returns physical address stored in the wrapped hazard pointer
        typename AllocatorInstantiation::address_type get() const
        {
            return static_cast<HPListEntry*>(m_p_hp_entry)->value;
        }

    private:
        HazardPointerRecord(void* p_hp_entry) :
            m_p_hp_entry{ p_hp_entry },
            m_ref_counter{ new uint32_t{ 1U } }
        {
            static_cast<HPListEntry*>(m_p_hp_entry)->is_active.store(true, std::memory_order::memory_order_release);
        }

        uint32_t* m_ref_counter;    //!< reference counter tracking, whether the wrapped hazard pointer is still in use
        void* m_p_hp_entry;    //!< address of the hazard pointer entry in the cache wrapped by the record object
    };



    HazardPointerPool(AllocatorInstantiation& allocator)
        : m_allocator{ allocator }
        , m_gc_threshold{ 24U }
        , m_hp_list_head { new HPListEntry{} }
        , m_hp_list_tail{ m_hp_list_head }

#ifdef _DEBUG
        , m_num_hps{ 1U }
        , m_num_buffers_freed{ 0U }
#endif

    {

    }

    ~HazardPointerPool()
    {
        // remove hazard pointer pool cache
        HPListEntry* p_hp_next;
        while (p_hp_next = m_hp_list_head->next.load(std::memory_order_acquire))
        {
            delete m_hp_list_head;
            m_hp_list_head = p_hp_next;
        }
        delete m_hp_list_head;

        cleanup();
    }

    HazardPointerPool(HazardPointerPool const&) = delete;
    HazardPointerPool(HazardPointerPool&&) = delete;

    HazardPointerPool& operator=(HazardPointerPool const&) = delete;
    HazardPointerPool& operator=(HazardPointerPool&&) = delete;


    //! acquires new hazard pointer for the calling thread and uses it to protect the provided raw pointer value.
    HazardPointerRecord acquire(typename AllocatorInstantiation::address_type const& ptr_value)
    {
        // First we check if there are unused pointers in the list
        for (HPListEntry* p_entry = m_hp_list_head; p_entry != nullptr; p_entry = p_entry->next.load(std::memory_order::memory_order_acquire))
        {
            bool is_active = p_entry->is_active.load(std::memory_order::memory_order_consume);
            if (!is_active)
            {
                if (p_entry->is_active.compare_exchange_strong(is_active, true, std::memory_order::memory_order_acq_rel))
                {
                    p_entry->value = ptr_value;
                    return HazardPointerRecord{ p_entry };
                }
            }
        }


        // We did not manage to reuse one of the older HP-buckets. Therefore, we need to allocate a new one.
        HPListEntry* p_new_bucket = new HPListEntry{};
        p_new_bucket->value = ptr_value;
        HazardPointerRecord rv{ p_new_bucket };    // we need to create the hp record to be returned already here to make sure that the wrapped HP is active

        while (true)
        {
            HPListEntry* p_current_tail = m_hp_list_tail.load(std::memory_order::memory_order_consume);
            HPListEntry* p_next = p_current_tail->next.load(std::memory_order::memory_order_consume);

            if (p_next != nullptr)
            {
                // The tail has moved, try to update it
                m_hp_list_tail.compare_exchange_weak(p_current_tail, p_next, std::memory_order::memory_order_acq_rel);

                // Whether we managed to update the tail or not, we can only proceed trying to add the new element into the list...
            }
            else
            {
                // The tail did actually point to the end of the queue, but we cannot be sure any longer...

                // We try to update the tail so that it gets connected to the new bucket
                if (p_current_tail->next.compare_exchange_strong(p_next, p_new_bucket, std::memory_order::memory_order_acq_rel))
                {
                    // If we have managed to attach the new bucket, try to move the tail forward
                    m_hp_list_tail.compare_exchange_weak(p_current_tail, p_new_bucket, std::memory_order::memory_order_acq_rel);

#ifdef _DEBUG
                    ++m_num_hps;    // increment hazard pointer counter (used for debug purposes)
#endif

                    return rv;
                }
            }
        }
    }

    //! the provided hazard pointer is marked for deletion by the calling thread
    void retire(HazardPointerRecord const& hp_record)
    {
        __addEntryToLocalGCList(m_dlist_tail, static_cast<HPListEntry*>(hp_record.m_p_hp_entry));
        ++m_dlist_cardinality;

        if (m_dlist_cardinality > m_gc_threshold)
        {
            __scan();
        }
    }


    //! forces the garbage collector to free up all memory blocks remaining in deletion cache of the calling thread
    void flush()
    {
        // flush only if there is something to flush
        if (m_dlist_cardinality)
        {
            __scan();
        }
    }

    /*!
     Sets minimal amount of hazard pointer records that has to reside in deletion cache of a certain thread in order to initiate
     garbage collection process
    */
    void setGCThreshold(uint32_t threshold)
    {
        m_gc_threshold = threshold;
    }


    /*!
     Cleans up the hazard pointer pool. This function must be called on each thread that was using the HP-pool before the thread is about to exit.
     After this function is called the HP-pool cannot be longer used on the calling thread
    */
    void cleanup()
    {
        flush();    // remove data that might still be in the dlist

        // clear dlist and plist caches
        GCListEntry* p_pd_next;
        while (p_pd_next = m_dlist_head->p_next)
        {
            delete m_dlist_head;
            m_dlist_head = p_pd_next;
        }
        delete m_dlist_head;

        while (p_pd_next = m_plist_head->p_next)
        {
            delete m_plist_head;
            m_plist_head = p_pd_next;
        }
        delete m_plist_head;
    }


private:
    //! Describes single entry in the hazard pointer list
    struct HPListEntry
    {
        typename AllocatorInstantiation::address_type value;    //!< actual value encapsulated by the hazard pointer
        std::atomic_bool is_active;    //!< 'true' if the pointer is valid; 'false' if it has been deallocated
        std::atomic<HPListEntry*> next;    //!< atomic pointer to the next entry in the hazard pointer list

        HPListEntry() :
            value{ nullptr },
            is_active{ false },
            next{ nullptr }
        {

        }
        HPListEntry(HPListEntry const&) = delete;
        HPListEntry(HPListEntry&&) = delete;
        HPListEntry& operator=(HPListEntry const&) = delete;
        HPListEntry& operator=(HPListEntry&&) = delete;
        ~HPListEntry() = default;
    };

    //! Describes elements of the GC-lists owned by the thread
    struct GCListEntry
    {
        typename AllocatorInstantiation::address_type p_mem_block;    //!< pointer to the memory block, which is subject for deletion
        GCListEntry* p_next;    //!< pointer to the next element in the list
        bool is_in_use;    //!< 'true' if the entry of the garbage collector is in use; 'false' if it is available for reuse.
    };


    inline void __addEntryToLocalGCList(GCListEntry*& p_list_tail, HPListEntry* p_hp_entry)
    {
        while (p_list_tail->p_next && p_list_tail->p_next->is_in_use)
            p_list_tail = p_list_tail->p_next;

        if (!p_list_tail->p_next)
        {
            p_list_tail->p_next = new GCListEntry{ p_hp_entry->value, nullptr, true };
            p_list_tail = p_list_tail->p_next;
        }
        else
        {
            p_list_tail->p_next->p_mem_block = p_hp_entry->value;
            p_list_tail->p_next->is_in_use = true;
            p_list_tail = p_list_tail->p_next;
        }
    }

    inline void __scan()
    {
        // now it is time to remove the pointers that are not in "hazardous" state from the delete list

        // formate the list of pointers that are currently in "hazardous" state
        for (HPListEntry* p_hp_list_entry = m_hp_list_head; p_hp_list_entry != nullptr; p_hp_list_entry = p_hp_list_entry->next.load(std::memory_order::memory_order_consume))
        {
            if (p_hp_list_entry->is_active)
                __addEntryToLocalGCList(m_plist_tail, p_hp_list_entry);
        }

        // for each pointer marked for deletion check if this pointer is in the list of pointers in "hazardous" state. If so, deallocate the corresponding memory block
        uint32_t num_freed_successfully{ 0U };
        for (GCListEntry* p_dlist_entry = m_dlist_head->p_next; p_dlist_entry != nullptr; p_dlist_entry = p_dlist_entry->p_next)
        {
            if (!p_dlist_entry->is_in_use || !p_dlist_entry->p_mem_block)
                continue;

            bool deallocation_possible = true;
            for (GCListEntry* p_plist_entry = m_plist_head->p_next;
                p_plist_entry != m_plist_tail->p_next;
                p_plist_entry = p_plist_entry->p_next)
            {
                if (p_plist_entry->p_mem_block == p_dlist_entry->p_mem_block)
                {
                    deallocation_possible = false;
                    break;
                }
            }

            if (deallocation_possible)
            {
                m_allocator.free(p_dlist_entry->p_mem_block);
                ++num_freed_successfully;

#ifdef _DEBUG
                ++m_num_buffers_freed;
#endif

                p_dlist_entry->is_in_use = false;
            }
        }


        // reset dlist
        m_dlist_tail = m_dlist_head;
        m_dlist_cardinality -= num_freed_successfully;

        // reset plist
        for (GCListEntry* p_plist_entry = m_plist_head->p_next; p_plist_entry != m_plist_tail->p_next; p_plist_entry = p_plist_entry->p_next)
        {
            p_plist_entry->is_in_use = false;
        }
        m_plist_tail = m_plist_head;
    }



    AllocatorInstantiation& m_allocator;    //!< reference to the allocator object used to free unused data

    uint32_t m_gc_threshold;    //!< garbage collection threshold. The default value is 24 (since lock-free algorithms normally don't use more than 3 hazard pointers and the modern CPUs have no more than 8 cores)

    HPListEntry* m_hp_list_head;    //!< "head" of the hazard pointer list. This value never changes after initialization
    std::atomic<HPListEntry*> m_hp_list_tail;    //!< "tail" of the hazard pointer list. This value gets updated in lock-free style

    static thread_local GCListEntry* m_dlist_head, *m_dlist_tail;
    static thread_local GCListEntry* m_plist_head, *m_plist_tail;

    static thread_local uint32_t m_dlist_cardinality;

#ifdef _DEBUG
    std::atomic_uint32_t m_num_hps;    //!< number of hazard pointers residing in the cache
    std::atomic_uint32_t m_num_buffers_freed;    //!< total number of buffers deallocated
#endif
};


template<typename AllocatorInstance>
thread_local typename HazardPointerPool<AllocatorInstance>::GCListEntry* HazardPointerPool<AllocatorInstance>::m_dlist_head = new HazardPointerPool<AllocatorInstance>::GCListEntry{ nullptr, nullptr, false };

template<typename AllocatorInstance>
thread_local typename HazardPointerPool<AllocatorInstance>::GCListEntry* HazardPointerPool<AllocatorInstance>::m_dlist_tail = { m_dlist_head };

template<typename AllocatorInstance>
thread_local uint32_t HazardPointerPool<AllocatorInstance>::m_dlist_cardinality = 0U;



template<typename AllocatorInstance>
thread_local typename HazardPointerPool<AllocatorInstance>::GCListEntry* HazardPointerPool<AllocatorInstance>::m_plist_head = new HazardPointerPool<AllocatorInstance>::GCListEntry{ nullptr, nullptr, false };

template<typename AllocatorInstance>
thread_local typename HazardPointerPool<AllocatorInstance>::GCListEntry* HazardPointerPool<AllocatorInstance>::m_plist_tail = { m_plist_head };



}}}

#define LEXGINE_CORE_CONCURRENCY_HAZARD_POINTER_POOL_H
#endif
