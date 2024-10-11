#ifndef LEXGINE_CORE_ALLOCATOR_H
#define LEXGINE_CORE_ALLOCATOR_H

#include <utility>

namespace lexgine {namespace core {

//! Base class for all allocators
template<typename T>
class Allocator
{
private:
    //! Base class for pointers returned by allocators
    template<typename T>
    class MemoryBlock
    {
    public:
        template<typename ... Args>
        inline MemoryBlock(Args&&... args) :
            obj{ std::forward<Args>(args)... }
        {

        }

    public:
        inline T* operator->() { return &obj; }
        inline T const* operator->() const { return &obj; }

    private:
        T obj;    //!< allocated object instance
    };

    /*! Wraps memory block into and object-like pointer representation to
    allow natural access the member "obj" encapsulated by MemoryBlock instance
    */
    template<typename T>
    class MemoryBlockAddr final
    {
        friend class Allocator<T>;

    public:

        MemoryBlockAddr(MemoryBlock<T>* p_memory_block) :
            m_mem_block_addr{ p_memory_block }
        {

        }

        explicit MemoryBlockAddr(size_t addr) :
            m_mem_block_addr{ reinterpret_cast<MemoryBlock<T>*>(addr) }
        {

        }

        inline MemoryBlockAddr& operator=(MemoryBlockAddr const& other)
        {
            m_mem_block_addr = other.m_mem_block_addr;
            return *this;
        }

        inline MemoryBlockAddr& operator=(MemoryBlock<T>* pBlock)
        {
            m_mem_block_addr = pBlock;
            return *this;
        }

        inline MemoryBlock<T>& operator->() { return *m_mem_block_addr; }
        inline MemoryBlock<T> const& operator->() const { return *m_mem_block_addr; }

        inline bool operator == (MemoryBlockAddr<T> const& other) const { return m_mem_block_addr == other.m_mem_block_addr; }
        inline bool operator == (void* other_pointer) const { return m_mem_block_addr == other_pointer; }
        inline bool operator != (MemoryBlockAddr<T> const& other) const { return m_mem_block_addr != other.m_mem_block_addr; }
        inline bool operator != (void* other_pointer) const { return m_mem_block_addr != other_pointer; }
        inline operator bool() const { return m_mem_block_addr != nullptr; }
        inline explicit operator size_t() const { return reinterpret_cast<size_t>(m_mem_block_addr); }

    private:
        MemoryBlock<T>* m_mem_block_addr;
    };

public:
    using value_type = T;
    using memory_block_type = MemoryBlock<T>;
    using address_type = MemoryBlockAddr<T>;

    virtual ~Allocator() = default;

protected:
    //! Allows to cast MemoryBlockAddr<T> objects to MemoryBlock<T> object pointers, which is a useful operation when implementing allocators
    inline MemoryBlock<T>* pointerCast(MemoryBlockAddr<T> const& pointer_wrapper)
    {
        return pointer_wrapper.m_mem_block_addr;
    }
};


}}

#endif
