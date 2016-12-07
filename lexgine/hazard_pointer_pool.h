#ifndef LEXGINE_CORE_CONCURRENCY_HAZARD_POINTER_POOL_H

#include <cstdint>
#include <memory>
#include <atomic>

namespace lexgine {namespace core {namespace concurrency {

//! Pool of hazard pointers. Allows to make a given allocation hazardous, which prevents it from being freed by the concurrent threads
class HazardPointerPool
{
public:
    //! Describes a hazard pointer
    class HazardPointer
    {
        friend class HazardPointerPool;

    public:
        ~HazardPointer() = default;

        void* get() const;    //! returns the actual value of the pointer
        bool isActive() const;    //! returns 'true' if the pointer is active. Returns 'false' otherwise
        bool isHazardous() const;    //! returns 'true' if the pointer record has been set "hazardous" by some thread. Returns 'false' otherwise

        void setHazardous();    //! sets the pointer to the "hazardous" state. While the pointer stays in this state, the data it points to will never be removed.
        void setSafeToRemove();    //! removes the "hazardous" state from the pointer. It is not safe to access the data through this pointer when it is not hazardous.

    private:
        HazardPointer();
        HazardPointer(HazardPointer const&) = delete;
        HazardPointer(HazardPointer&&) = delete;
        HazardPointer& operator=(HazardPointer const&) = delete;
        HazardPointer& operator=(HazardPointer&&) = delete;


        void* m_value;    //!< actual value encapsulated by the hazard pointer
        std::atomic_bool m_is_active;    //!< 'true' if the pointer is valid; 'false' if it has been deallocated
        bool m_is_hazardous;    //!< 'true' if the pointer has been set "hazardous" by some thread; 'false' otherwise
    };


    HazardPointerPool();
    ~HazardPointerPool();

    HazardPointerPool(HazardPointerPool const&) = delete;
    HazardPointerPool(HazardPointerPool&&) = delete;

    HazardPointerPool& operator=(HazardPointerPool const&) = delete;
    HazardPointerPool& operator=(HazardPointerPool&&) = delete;

    /*! acquires new hazard pointer for the calling thread and uses it to protect the provided raw pointer value.
     The returned hazard pointer record is supposed to be manipulated only by the thread that called this function
    */
    HazardPointer* acquire(void* ptr_value);

    void retire(HazardPointer* p_hp);    //! the provided hazard pointer is marked for deletion by the calling thread

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
