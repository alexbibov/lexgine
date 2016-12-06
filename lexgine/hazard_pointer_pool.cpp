#include "hazard_pointer_pool.h"

#include <cassert>


using namespace lexgine::core::concurrency;



void* HazardPointerPool::HazardPointer::get() const
{
    return m_value;
}

bool HazardPointerPool::HazardPointer::isActive() const
{
    return m_is_active;
}

bool HazardPointerPool::HazardPointer::isHazardous() const
{
    return m_is_hazardous;
}

void HazardPointerPool::HazardPointer::setHazardous()
{
    m_is_hazardous = true;
}

void HazardPointerPool::HazardPointer::setSafeToRemove()
{
    m_is_hazardous = false;
}

HazardPointerPool::HazardPointer::HazardPointer():
    m_value{ nullptr },
    m_is_active{ false },
    m_is_hazardous{ false }
{

}




class HazardPointerPool::impl
{
public:
    impl() :
        m_hp_list_head{ new HPListEntry{} },
        m_hp_list_tail{ m_hp_list_head },
        m_num_hps{ 0U }
    {
        //std::atomic_init(&m_hp_list_head->next, nullptr);
        //std::atomic_init(&m_hp_list_tail, m_hp_list_head);
    }

    HazardPointer* acquire(void* ptr_value)
    {
        // First we check if there are unused pointers in the list
        for (HPListEntry* p_entry = m_hp_list_head; p_entry != nullptr; p_entry = p_entry->next.load(std::memory_order::memory_order_consume))
        {
            bool is_active = p_entry->hp.m_is_active.load(std::memory_order::memory_order_consume);
            if (!is_active)
            {
                if (p_entry->hp.m_is_active.compare_exchange_strong(is_active, true, std::memory_order::memory_order_acq_rel))
                {
                    p_entry->hp.m_value = ptr_value;
                    return &p_entry->hp;
                }
            }
        }


        // We did not manage to reuse one of the older HP-buckets. Therefore, we need to allocate a new one.
        HPListEntry* p_new_bucket = new HPListEntry{};
        p_new_bucket->hp.m_is_active = true;
        p_new_bucket->hp.m_value = ptr_value;
        //std::atomic_init(&p_new_bucket->next, nullptr);

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
                    m_hp_list_tail.compare_exchange_weak(p_current_tail, p_next, std::memory_order::memory_order_acq_rel);



                    return &p_new_bucket->hp;
                }
            }
        }
    }


    void retire(HazardPointer* p_hp)
    {
        addEntryToLocalGCList(m_dlist_head, m_dlist_tail, p_hp);
        ++m_dlist_cardinality;

        if (m_dlist_cardinality > m_num_hps)
        {
            // now it is time to remove the pointers that are not in "hazardous" state from the delete list

            // formate the list of pointers that are currently in "hazardous" state
            for (HPListEntry* p_hp_list_entry = m_hp_list_head; p_hp_list_entry != nullptr; p_hp_list_entry = p_hp_list_entry->next.load(std::memory_order::memory_order_consume))
            {
                if (p_hp_list_entry->hp.m_is_hazardous)
                    addEntryToLocalGCList(m_plist_head, m_plist_tail, &p_hp_list_entry->hp);
            }

            // for each pointer marked for deletion check if this pointer is in the list of pointers in "hazardous" state. If so, deallocate the corresponding memory block
            for (GCListEntry* p_dlist_entry = m_dlist_head->p_next; p_dlist_entry != m_dlist_tail->p_next; p_dlist_entry = p_dlist_entry->p_next)
            {
                bool deallocation_possible = true;
                for (GCListEntry* p_plist_entry = m_plist_head->p_next; p_plist_entry != m_plist_tail->p_next; p_plist_entry = p_plist_entry->p_next)
                {
                    if (p_plist_entry->p_hp_list_entry == p_dlist_entry->p_hp_list_entry)
                    {
                        deallocation_possible = false;
                        break;
                    }
                }

                if (deallocation_possible)
                {
                    free(p_dlist_entry->p_hp_list_entry->m_value);
                    p_dlist_entry->is_in_use = false;

                    p_dlist_entry->p_hp_list_entry->m_is_active = false;    // the hazard pointer stops being active
                }
            }


            // reset dlist
            m_dlist_tail = m_dlist_head;
            m_dlist_cardinality = 0U;

            // reset plist
            for (GCListEntry* p_plist_entry = m_plist_head->p_next; p_plist_entry != m_plist_tail->p_next; p_plist_entry = p_plist_entry->p_next)
            {
                p_plist_entry->is_in_use = false;
            }
            m_plist_tail = m_plist_head;
        }
    }


    ~impl()
    {
        // Begin by clearing the hazard pointer list. Note that all the pointers should be in "safe to remove" state by the time the destructor is called
        {
            HPListEntry* p_hp_list_entry = m_hp_list_head;
            while (p_hp_list_entry)
            {
                assert(!p_hp_list_entry->hp.m_is_hazardous);

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
    //! Describes single entry in the hazard pointer list
    struct HPListEntry
    {
        HazardPointer hp;    //!< hazard pointer descriptor
        std::atomic<HPListEntry*> next;    //!< atomic pointer to the next entry in the hazard pointer list

        HPListEntry() :
            hp{},
            next{ nullptr }
        {

        }
    };


    //! Describes elements of the GC-lists owned by the thread
    struct GCListEntry
    {
        HazardPointer* p_hp_list_entry;    //!< pointer to the hazard pointer descriptor to be "watched" by the garbage collector
        GCListEntry* p_next;    //!< pointer to the next element in the list
        bool is_in_use;    //!< 'true' if the entry of the garbage collector is in use; 'false' if it is available for reuse.
    };


    inline void addEntryToLocalGCList(GCListEntry*& p_list_head, GCListEntry*& p_list_tail, HazardPointer* p_hp)
    {
        if (!p_list_tail)
            p_list_tail = p_list_head;

        while (p_list_tail->p_next && p_list_tail->p_next->is_in_use)
            p_list_tail = p_list_tail->p_next;

        if (!p_list_tail->p_next)
        {
            p_list_tail->p_next = new GCListEntry{ p_hp, nullptr, true };
            p_list_tail = p_list_tail->p_next;
        }
        else
        {
            p_list_tail->p_next->p_hp_list_entry = p_hp;
            p_list_tail->p_next->is_in_use = true;
            p_list_tail = p_list_tail->p_next;
        }
    }


    HPListEntry* m_hp_list_head;    //!< "head" of the hazard pointer list. This value never changes after initialization
    std::atomic<HPListEntry*> m_hp_list_tail;    //!< "tail" of the hazard pointer list. This value gets updated in lock-free style
    uint32_t m_num_hps;    //!< number of hazard pointers in the list. This value is just an estimator and does not provide exact number of currently created hazard pointers

    static thread_local GCListEntry* m_dlist_head, *m_dlist_tail;
    static thread_local GCListEntry* m_plist_head, *m_plist_tail;

    static thread_local uint32_t m_dlist_cardinality;
};

thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_dlist_head = new HazardPointerPool::impl::GCListEntry{ nullptr, nullptr, true };
thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_dlist_tail = nullptr;
thread_local uint32_t HazardPointerPool::impl::m_dlist_cardinality = 0U;

thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_plist_head = new HazardPointerPool::impl::GCListEntry{ nullptr, nullptr, true };
thread_local HazardPointerPool::impl::GCListEntry* HazardPointerPool::impl::m_plist_tail = nullptr;




HazardPointerPool::HazardPointerPool():
    m_impl{ new impl{} }
{
}

HazardPointerPool::~HazardPointerPool() = default;

HazardPointerPool::HazardPointer* HazardPointerPool::acquire(void* ptr_value)
{
    return m_impl->acquire(ptr_value);
}

void HazardPointerPool::retire(HazardPointer* p_hp)
{
    m_impl->retire(p_hp);
}