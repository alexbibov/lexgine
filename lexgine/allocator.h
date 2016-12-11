#ifndef LEXGINE_CORE_ALLOCATOR_H

#include <cstdint>

namespace lexgine {namespace core {

//! Abstraction over generic memory allocator. This class is OS- and API- agnostic
class Allocator
{
public:
    //! Initializes new allocator with the given amount of maximal available memory
    Allocator(size_t max_memory_available = 4096U);    
    Allocator(Allocator const&) = delete;
    Allocator(Allocator&&) = delete;

    Allocator& operator=(Allocator const&) = delete;
    Allocator& operator=(Allocator&&) = delete;

    virtual ~Allocator() = default;

    void* allocate(size_t size);    //! allocates new memory block of provided size. If a memory block of requested size cannot be allocated, returns nullptr
    bool free(void* mem_block);    //! deallocates given memory block. Returns 'true' on success and 'false' if the given memory block was already deallocated
    virtual size_t getMemoryBlockSize(void* mem_block) const = 0;    //! returns size of provided memory block

    size_t getTotalMemory() const;    //! returns total amount of memory (in bytes) available for allocation using this allocator
    size_t getFreeMemory() const;    //! returns amount of memory (in bytes), which is currently available for allocation using this allocator
    size_t getUsedMemory() const;    //! returns amount of memory (in bytes) currently used by this allocator

protected:
    virtual void* performAllocation(size_t size) = 0;    //! performs actual allocation of a memory block of requested size. Must return nullptr in case of failure.
    virtual bool performDeallocation(void* mem_block) = 0;    //! deallocates given memory block. Must return 'true' on success and 'false' if requested memory block was already deallocated.

private:
    size_t const m_max_available_memory;    //!< maximal amount of memory provided by the allocator
    size_t m_used_memory;    //!< currently used amount of memory
};

}}

#define LEXIGNE_CORE_ALLOCATOR_H
#endif
