#ifndef LEXGINE_CORE_ALLOCATOR_H
#define LEXGINE_CORE_ALLOCATOR_H

#include <cassert>
#include <cstddef>
#include <utility>
#include <atomic>
#include "engine/core/misc/optional.h"

namespace lexgine::core {


//! Base class for all allocators
template<typename T>
class Allocator
{
public:
    using value_type = T;

    //! Base class for memory blocks returned by allocators
    class memory_block_type 
    {
    public:
        template <typename... Args>
        inline memory_block_type(Args&&... args)
            : m_obj { std::forward<Args>(args)... }
        {
        }

    public:
        inline T* operator->() { return &m_obj; }
        inline T const* operator->() const { return &m_obj; }

    protected:
        void freeInternal()
        {
            m_obj.~T();
        }

    private:
        T m_obj; //!< allocated object instance
    };

    
    /*! Wraps memory block into and object-like pointer representation to
    allow natural access the member "obj" encapsulated by memory_block_type instance.
    This pointer representation does not necessarily have to be a pointer in the traditional sense, it
    may instead be an index into a memory pool or a handle to a memory block in a memory manager.
    */
    template <typename U>
    class t_address_type {
    public:
        using value_type = U;

    public:
        t_address_type(Allocator* p_allocator)
            : m_allocator_ptr{ p_allocator }
            , m_is_valid{ false }
        {
        }

        t_address_type(U const& value, Allocator* p_allocator)
            : m_allocator_ptr { p_allocator }
            , m_is_valid{ true }
            , m_opaque_memory_block_pointer{ value }
        {
        }

        operator bool() const
        {
            return m_is_valid;
        }

        explicit operator U() const
        {
            return m_opaque_memory_block_pointer;
        }

        t_address_type& operator=(U const& value)
        {
            m_is_valid = true;
            m_opaque_memory_block_pointer = value;
            return *this;
        }

        t_address_type& operator=(t_address_type const& other)
        {
            assert(m_allocator_ptr == other.m_allocator_ptr);

            if (this == &other)
                return *this;

            m_is_valid = other.m_is_valid;
            m_opaque_memory_block_pointer = other.m_opaque_memory_block_pointer;
            return *this;
        }

        bool operator==(t_address_type const& other) const = default;

        memory_block_type& operator->()
        {
            return *get();
        }

        memory_block_type const& operator->() const
        {
            return *get();
        }

        virtual memory_block_type* get() = 0;

        memory_block_type const* get() const
        {
            return const_cast<t_address_type*>(this)->get();
        }

    protected:
        Allocator* m_allocator_ptr { nullptr };
        bool m_is_valid;
        U m_opaque_memory_block_pointer;
    };

    template <>
    class t_address_type<memory_block_type*> final
    {
    public:
        using value_type = memory_block_type*;

    public:
        t_address_type() = default;

        t_address_type(memory_block_type* memory_block_pointer)
            : m_memory_block_pointer { memory_block_pointer }
        {
        }

        t_address_type& operator=(t_address_type const&) = default;

        t_address_type& operator=(memory_block_type* other)
        {
            m_memory_block_pointer = other;
            return *this;
        }

        bool operator==(t_address_type const&) const = default;

        memory_block_type& operator->() { return *m_memory_block_pointer; }
        memory_block_type const& operator->() const { return const_cast<t_address_type*>(this)->operator->(); }

        explicit operator uintptr_t() const { return reinterpret_cast<uintptr_t>(m_memory_block_pointer); }
        operator bool() const { return m_memory_block_pointer != nullptr; }

        memory_block_type* get() { return m_memory_block_pointer; }
        memory_block_type const* get() const { return const_cast<t_address_type*>(this)->get(); }

    private:
        memory_block_type* m_memory_block_pointer;
    };

public:
    virtual ~Allocator() = default;
};

}

#endif
