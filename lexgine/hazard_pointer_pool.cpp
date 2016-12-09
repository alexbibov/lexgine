#include "hazard_pointer_pool.h"

#include <cassert>


using namespace lexgine::core::concurrency;



class HazardPointerPool::impl
{
public:

    //! Describes single entry in the hazard pointer list
    struct HPListEntry
    {
        void* value;    //!< actual value encapsulated by the hazard pointer
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



    impl(uint32_t gc_threshold) :
        m_hp_list_head{ new HPListEntry{} }
        , m_hp_list_tail{ m_hp_list_head }
        , m_gc_threshold{ gc_threshold }

#ifdef _DEBUG
        , m_num_hps{ 1U }
        , m_num_buffers_freed{ 0U }
#endif

    {

    }

    HazardPointerRecord acquire(void* ptr_value)
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
                if (p_current_tail->next.compare_exchange_weak(p_next, p_new_bucket, std::memory_order::memory_order_acq_rel))
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


    void retire(HPListEntry* p_hp)
    {
        addEntryToLocalGCList(m_dlist_head, m_dlist_tail, p_hp);
        ++m_dlist_cardinality;

        if (m_dlist_cardinality > m_gc_threshold)
        {
            scan();
        }
    }


    void flush()
    {
        // flush only if there is something to flush
        if (m_dlist_cardinality)
        {
            scan();
        }
    }


    ~impl()
    {
        // Begin by clearing the hazard pointer list. Note that all the pointers should be in "safe to remove" state by the time the destructor is called
        {
            HPListEntry* p_hp_list_entry = m_hp_list_head;
            while (p_hp_list_entry)
            {
                assert(!p_hp_list_entry->is_active.load(std::memory_order::memory_order_consume));

                HPListEntry* p_aux = p_hp_list_entry;
                p_hp_list_entry = p_hp_list_entry->next.load(std::memory_order::memory_order_consume);
                delete p_aux;
            }
        }

        // Deallocate dlist cache
        {
            GCListEntry* p_dlist_entry = m_dlist_head;
            while (p_dlist_entry)
            {
                assert(!p_dlist_entry->is_in_use);

                GCListEntry* p_aux = p_dlist_entry;
                p_dlist_entry = p_dlist_entry->p_next;
                delete p_aux;
            }
        }

        // Deallocate plist cache
        {
            GCListEntry* p_plist_entry = m_plist_head;
            while (p_plist_entry)
            {
                GCListEntry* p_aux = p_plist_entry;
                p_plist_entry = p_plist_entry->p_next;

                delete p_aux;
            }
        }
    }




private:

    //! Describes elements of the GC-lists owned by the thread
    struct GCListEntry
    {
        void* p_mem_block;    //!< pointer to the memory block, which is subject for deletion
        GCListEntry* p_next;    //!< pointer to the next element in the list
        bool is_in_use;    //!< 'true' if the entry of the garbage collector is in use; 'false' if it is available for reuse.
    };



    inline void addEntryToLocalGCList(GCListEntry*& p_list_head, GCListEntry*& p_list_tail, HPListEntry* p_hp_entry)
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

    inline void scan()
    {
        // now it is time to remove the pointers that are not in "hazardous" state from the delete list

        // formate the list of pointers that are currently in "hazardous" state
        for (HPListEntry* p_hp_list_entry = m_hp_list_head; p_hp_list_entry != nullptr; p_hp_list_entry = p_hp_list_entry->next.load(std::memory_order::memory_order_consume))
        {
            if (p_hp_list_entry->is_active)
                addEntryToLocalGCList(m_plist_head, m_plist_tail, p_hp_list_entry);
        }

        // for each pointer marked for deletion check if this pointer is in the list of pointers in "hazardous" state. If so, deallocate the corresponding memory block
        uint32_t num_freed_successfully{ 0U };
        for (GCListEntry* p_dlist_entry = m_dlist_head->p_next; p_dlist_entry != nullptr; p_dlist_entry = p_dlist_entry->p_next)
        {
            if(!p_dlist_entry->is_in_use || !p_dlist_entry->p_mem_block)
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
                free(p_dlist_entry->p_mem_block);
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



    HPListEntry* m_hp_list_head;    //!< "head" of the hazard pointer list. This value never changes after initialization
    std::atomic<HPListEntry*> m_hp_list_tail;    //!< "tail" of the hazard pointer list. This value gets updated in lock-free style
    uint32_t m_gc_threshold;    //!< garbage collection threshold

    static thread_local GCListEntry* m_dlist_head, *m_dlist_tail;
    static thread_local GCListEntry* m_plist_head, *m_plist_tail;

    static thread_local uint32_t m_dlist_cardinality;

#ifdef _DEBUG
    std::atomic_uint32_t m_num_hps;    //!< number of hazard pointers residing in the cache
    std::atomic_uint32_t m_num_buffers_freed;    //!< total number of buffers deallocated
#endif
};

thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_dlist_head = new HazardPointerPool::impl::GCListEntry{ nullptr, nullptr, false };
thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_dlist_tail = { m_dlist_head };
thread_local uint32_t HazardPointerPool::impl::m_dlist_cardinality = 0U;

thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_plist_head = new HazardPointerPool::impl::GCListEntry{ nullptr, nullptr, false };
thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_plist_tail = { m_plist_head };





HazardPointerPool::HazardPointerPool():
    m_gc_threshold{ 24U },
    m_impl{ new impl{m_gc_threshold} }
{
}

HazardPointerPool::~HazardPointerPool() = default;

HazardPointerPool::HazardPointerRecord HazardPointerPool::acquire(void* ptr_value)
{
    return m_impl->acquire(ptr_value);
}

void HazardPointerPool::retire(HazardPointerRecord const& hp_record)
{
    m_impl->retire(static_cast<impl::HPListEntry*>(hp_record.m_p_hp_entry));
}

void lexgine::core::concurrency::HazardPointerPool::flush()
{
    m_impl->flush();
}

void HazardPointerPool::setGCThreshold(uint32_t threshold)
{
    m_gc_threshold = threshold;
}

HazardPointerPool::HazardPointerRecord::HazardPointerRecord():
    m_p_hp_entry{ nullptr },
    m_ref_counter{ nullptr }
{
}

HazardPointerPool::HazardPointerRecord::HazardPointerRecord(HazardPointerRecord const& other):
    m_p_hp_entry{ other.m_p_hp_entry },
    m_ref_counter{ other.m_ref_counter }
{
    ++(*m_ref_counter);
}

HazardPointerPool::HazardPointerRecord::HazardPointerRecord(HazardPointerRecord&& other):
    m_p_hp_entry{ other.m_p_hp_entry },
    m_ref_counter{ other.m_ref_counter }
{
    other.m_ref_counter = nullptr;
}

HazardPointerPool::HazardPointerRecord& HazardPointerPool::HazardPointerRecord::operator=(HazardPointerRecord const& other)
{
    if (this == &other)
        return *this;

    if (m_ref_counter && !(--(*m_ref_counter)))
    {
        delete m_ref_counter;
        static_cast<impl::HPListEntry*>(m_p_hp_entry)->is_active.store(false, std::memory_order::memory_order_release);
    }

    m_p_hp_entry = other.m_p_hp_entry;
    m_ref_counter = other.m_ref_counter;

    ++(*m_ref_counter);

    return *this;
}

HazardPointerPool::HazardPointerRecord& HazardPointerPool::HazardPointerRecord::operator=(HazardPointerRecord&& other)
{
    if (this == &other)
        return *this;

    if (m_ref_counter && !(--(*m_ref_counter)))
    {
        delete m_ref_counter;
        static_cast<impl::HPListEntry*>(m_p_hp_entry)->is_active.store(false, std::memory_order::memory_order_release);
    }

    m_p_hp_entry = other.m_p_hp_entry;
    m_ref_counter = other.m_ref_counter;
    other.m_ref_counter = nullptr;

    return *this;
}

HazardPointerPool::HazardPointerRecord::~HazardPointerRecord()
{
    if (m_ref_counter && !(--(*m_ref_counter)))
    {
        delete m_ref_counter;
        static_cast<impl::HPListEntry*>(m_p_hp_entry)->is_active.store(false, std::memory_order::memory_order_release);
    }
}

void* HazardPointerPool::HazardPointerRecord::get() const
{
    return static_cast<impl::HPListEntry*>(m_p_hp_entry)->value;
}



HazardPointerPool::HazardPointerRecord::HazardPointerRecord(void* p_hp_entry):
    m_p_hp_entry{ p_hp_entry },
    m_ref_counter{ new uint32_t{1U} }
{
    static_cast<impl::HPListEntry*>(m_p_hp_entry)->is_active.store(true, std::memory_order::memory_order_release);
}
