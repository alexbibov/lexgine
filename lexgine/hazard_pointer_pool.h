#ifndef LEXGINE_CORE_CONCURRENCY_HAZARD_POINTER_POOL_H

#include <cstdint>
#include <memory>
#include <atomic>

namespace lexgine {namespace core {namespace concurrency {

//! Pool of hazard pointers. Allows to make a given allocation hazardous, which prevents it from being freed by the concurrent threads
class HazardPointerPool
{
public:
    //! Describes hazard pointer record, which enables automatic reuse of hazard pointer entries in the cache
    class HazardPointerRecord
    {
        friend HazardPointerPool;

    public:
        HazardPointerRecord(HazardPointerRecord const& other);
        HazardPointerRecord(HazardPointerRecord&& other);
        HazardPointerRecord& operator=(HazardPointerRecord const& other);
        HazardPointerRecord& operator=(HazardPointerRecord&& other);
        ~HazardPointerRecord();

        void* get() const;    //! returns physical address stored in the wrapped hazard pointer

    private:
        HazardPointerRecord(void* p_hp_entry);

        uint32_t* m_ref_counter;    //!< reference counter tracking, whether the wrapped hazard pointer is still in use
        void* m_p_hp_entry;    //!< address of the hazard pointer entry in the cache wrapped by the record object
    };


    HazardPointerPool();
    ~HazardPointerPool();

    HazardPointerPool(HazardPointerPool const&) = delete;
    HazardPointerPool(HazardPointerPool&&) = delete;

    HazardPointerPool& operator=(HazardPointerPool const&) = delete;
    HazardPointerPool& operator=(HazardPointerPool&&) = delete;

    HazardPointerRecord acquire(void* ptr_value);    //! acquires new hazard pointer for the calling thread and uses it to protect the provided raw pointer value.

    void retire(HazardPointerRecord const& hp_record);    //! the provided hazard pointer is marked for deletion by the calling thread

    void flush();    //! forces the garbage collector to free up all memory blocks remaining in the deletion cache

    /*!
     Sets minimal amount of hazard pointer records that has to reside in deletion cache of a certain thread in order to initiate
     garbage collection process
    */
    void setGCThreshold(uint32_t threshold);

private:
    class impl;    //! conceals implementation details

    uint32_t m_gc_threshold;    //!< garbage collection threshold. The default value is 24 (since lock-free algorithms normally don't use more than 3 hazard pointers and the modern CPUs have no more than 8 cores)
    std::unique_ptr<impl> m_impl;    //!< pointer to the internal class encapsulating implementation details
};

}}}

#define LEXGINE_CORE_CONCURRENCY_HAZARD_POINTER_POOL_H
#endif
