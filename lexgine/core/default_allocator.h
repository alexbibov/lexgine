#ifndef LEXGINE_CORE_DEFAULT_ALLOCATOR_H

#include "allocator.h"

namespace lexgine {namespace core {

//! Implements default allocator for objects of type T (type T must implement default constructor)
template<typename T>
class DefaultAllocator : public Allocator<T>
{
public:
    //! Allocates new object of type T from the heap memory using default constructor
    address_type allocate()
    {
        return address_type{ new DefaultAllocatorMemoryBlock{} };
    }

    //! Removes the object instance at the address provided from the heap and guarantees that the object gets properly destructed
    void free(address_type const& memory_block_addr)
    {
        delete pointerCast(memory_block_addr);
    }


    size_t getBlockSize(address_type const& memory_block_addr)
    {
        return static_cast<DefaultAllocatorMemoryBlock*>(pointerCast(memory_block_addr))->size;
    }

private:
    struct DefaultAllocatorMemoryBlock : public MemoryBlock<T>
    {
        size_t size;    //!< size of the allocated block
    };
};

}}

#define LEXGINE_CORE_DEFAULT_ALLOCATOR_H
#endif
