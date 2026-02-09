#ifndef LEXGINE_CORE_RING_BUFFER_H
#define LEXGINE_CORE_RING_BUFFER_H

#include <atomic>
#include "engine/core/allocator.h"
#include "engine/core/misc/static_vector.h"

namespace lexgine::core {

/*! Ring buffer allocation manager. Note that ring buffer does not support
 constructors or destructors for type T. Therefore, it has to be used for primitive types only.
*/
template<typename T, size_t number_of_cells>
class RingBufferAllocatorN : public Allocator<T>
{
    friend class address_type;

private:
    using allocator_type = Allocator<T>;

    class RingBufferCell : public allocator_type::memory_block_type 
    {
    public:
        RingBufferCell()
            : m_is_used{ false }
        {

        }

        bool tryAcquire()
        {
            bool expected_usage{ false };
            return m_is_used.compare_exchange_strong(expected_usage, true, std::memory_order_acq_rel, std::memory_order_relaxed);
        }

        void free()
        {
            allocator_type::memory_block_type::freeInternal();
            m_is_used.store(false, std::memory_order_release);
        }
    private:
        std::atomic_bool m_is_used; //!< 'true' if the cell is currently in use, 'false' otherwise
    };

public:
    
    class address_type : public allocator_type::template t_address_type<uint64_t>
    {
    public:
        address_type(RingBufferAllocatorN* pAllocator)
            : allocator_type::template t_address_type<uint64_t>{0xffffffff, pAllocator}
        {

        }

        address_type(uint64_t value, RingBufferAllocatorN* pAllocator)
            : allocator_type::template t_address_type<uint64_t> { value, pAllocator }
        {

        }

        typename allocator_type::memory_block_type* get() override
        {
            size_t slot = getPointer();
            return &static_cast<RingBufferAllocatorN*>(allocator_type::template t_address_type<uint64_t>::m_allocator_ptr)->m_buffer[slot];
        }

        void setTag(uint32_t tag)
        {
            allocator_type::template t_address_type<uint64_t>::m_opaque_memory_block_pointer &= 0xffffffff;
            allocator_type::template t_address_type<uint64_t>::m_opaque_memory_block_pointer |= (static_cast<uint64_t>(tag) << 32);
        }

        uint32_t getPointer() const
        {
            return getPointer(allocator_type::template t_address_type<uint64_t>::m_opaque_memory_block_pointer);
        }

        uint32_t getTag() const
        {
            return getTag(allocator_type::template t_address_type<uint64_t>::m_opaque_memory_block_pointer);
        }

        static uint32_t getPointer(uint64_t pointer_bits)
        {
            return static_cast<uint32_t>(pointer_bits & 0xffffffff);
        }

        static uint32_t getTag(uint64_t pointer_bits)
        {
            return static_cast<uint32_t>(pointer_bits >> 32);
        }

        bool isValid() const
        {
            return getPointer() != 0xffffffff;
        }
    };

public:
    //! Initializes ring buffer with given number of memory cells
    RingBufferAllocatorN()
        : m_current_element{ 0 }
        , m_tag{ 0 }
    {
        for (size_t i = 0; i < number_of_cells; ++i) 
        {
            m_buffer.emplace_back();
        }
    }

    //! Allocates new object of type T from the ring buffer
    address_type allocate()
    {
        uint32_t element_index = m_current_element.load(std::memory_order_acquire);
        RingBufferCell* cell = m_buffer.data() + element_index;
        bool expected_usage{ false };
#ifdef _DEBUG
        size_t counter = 0;
#endif
        while (!cell->tryAcquire())
        {
            while (!m_current_element.compare_exchange_strong(
                element_index, 
                (element_index + 1) % number_of_cells, 
                std::memory_order_acq_rel, 
                std::memory_order_acquire));
            element_index = (element_index + 1) % number_of_cells;
            cell = m_buffer.data() + element_index;
            expected_usage = false;
#ifdef _DEBUG
            ++counter;
            assert(counter < number_of_cells);
#endif
        }
        return address_type{element_index, this};
    }

    uint32_t createTag()
    {
        return m_tag.fetch_add(1);
    }

    /*! Removes object having provided address from the ring buffer. If the input address does not point to a valid
     object of type T currently residing in the ring buffer the behavior is undefined.
    */
    void free(address_type& memory_block_addr)
    {
        RingBufferCell* p_cell = static_cast<RingBufferCell*>(memory_block_addr.get());
        p_cell->free();
        // p_cell->m_is_used.store(false, std::memory_order_release);
    }

    //! Returns full capacity of the ring buffer
    static constexpr size_t getCapacity() { return number_of_cells; }

private:
    std::atomic_uint32_t m_current_element;
    std::atomic_uint32_t m_tag;
    misc::StaticVector<RingBufferCell, number_of_cells> m_buffer;
};

template<typename T>
using RingBufferAllocator = RingBufferAllocatorN<T, 128>;

}

#endif
