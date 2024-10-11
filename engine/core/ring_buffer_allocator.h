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
public:
    //! Initializes ring buffer with given number of memory cells
    RingBufferAllocatorN()
    {
        for (size_t i = 0; i < number_of_cells; ++i) 
        {
            m_buffer.emplace_back();
            std::atomic_init(&m_buffer.back().is_used, false);
        }
        std::atomic_init(&m_current_element, 0);
    }

    //! Allocates new object of type T from the ring buffer
    typename Allocator<T>::address_type allocate()
    {
        size_t element_index = m_current_element.load(std::memory_order_acquire);
        RingBufferCell* cell = m_buffer.data() + element_index;
        bool expected_usage{ false };
        while (!cell->is_used.compare_exchange_strong(expected_usage, true, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            while (!m_current_element.compare_exchange_strong(element_index, (element_index + 1) % number_of_cells, std::memory_order_acq_rel, std::memory_order_acquire));
            cell = m_buffer.data() + (element_index + 1) % number_of_cells;
        }
        return typename Allocator<T>::address_type{ cell };
    }


    /*! Removes object having provided address from the ring buffer. If the input address does not point to a valid
     object of type T currently residing in the ring buffer the behavior is undefined.
    */
    void free(typename Allocator<T>::address_type const& memory_block_addr)
    {
        RingBufferCell* p_cell = static_cast<RingBufferCell*>(Allocator<T>::pointerCast(memory_block_addr));
        p_cell->is_used.store(false, std::memory_order_release);
    }

    //! Returns full capacity of the ring buffer
    static constexpr size_t getCapacity() { return number_of_cells; }

private:
    struct RingBufferCell : public Allocator<T>::memory_block_type
    {
        std::atomic_bool is_used;    //!< 'true' if the cell is currently in use, 'false' otherwise
    };

private:
    std::atomic_size_t m_current_element;
    misc::StaticVector<RingBufferCell, number_of_cells> m_buffer;
};

template<typename T>
using RingBufferAllocator = RingBufferAllocatorN<T, 512>;

}

#endif
