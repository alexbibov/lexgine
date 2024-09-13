#ifndef LEXGINE_CORE_RING_BUFFER_H
#define LEXGINE_CORE_RING_BUFFER_H

#include <atomic>
#include "engine/core/allocator.h"

namespace lexgine::core {

/*! Ring buffer allocation manager. Note that ring buffer does not support
 constructors or destructors for type T. Therefore, it has to be used for primitive types only.
*/
template<typename T>
class RingBufferAllocator : public Allocator<T>
{
public:
    //! Initializes ring buffer with given number of memory cells
    RingBufferAllocator(size_t number_of_cells = 128U) :
        m_num_of_cells{ number_of_cells }
    {
        if (number_of_cells)
        {
            m_buffer_ptr = new RingBufferCell{ false, nullptr };

            RingBufferCell* p_current_cell = m_buffer_ptr;
            for (size_t i = 0; i < number_of_cells - 1; ++i)
            {
                p_current_cell->p_next = new RingBufferCell{ false, nullptr };
                p_current_cell = p_current_cell->p_next;
            }
            p_current_cell->p_next = m_buffer_ptr;
        }
    }

    ~RingBufferAllocator()
    {
        RingBufferCell* p_current_cell = m_buffer_ptr;
        for (size_t i = 0; i < m_num_of_cells; ++i)
        {
            RingBufferCell* p_next = p_current_cell->p_next;
            delete p_current_cell;
            p_current_cell = p_next;
        }
    }

    //! Allocates new object of type T from the ring buffer
    typename Allocator<T>::address_type allocate()
    {
        RingBufferCell* p_current_cell = m_buffer_ptr;
        uint32_t num_parsed_count{ 0U };

        while (true)
        {
            while (num_parsed_count < m_num_of_cells && p_current_cell->is_used.load(std::memory_order_acquire))
            {
                p_current_cell = p_current_cell->p_next;
                ++num_parsed_count;
            }

            if (num_parsed_count == m_num_of_cells) return nullptr;    // the ring buffer is exhausted

            bool expected{ false };
            if (p_current_cell->is_used.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) break;
        }

        return typename Allocator<T>::address_type{ p_current_cell };
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
    size_t getCapacity() const { return m_num_of_cells; }

private:
    struct RingBufferCell : public Allocator<T>::memory_block_type
    {
        std::atomic_bool is_used;    //!< 'true' if the cell is currently in use, 'false' otherwise
        RingBufferCell* p_next;    //!< pointer to the next cell in the ring buffer

        RingBufferCell(bool is_used, RingBufferCell* p_next) :
            is_used{ is_used },
            p_next{ p_next }
        {

        }

    }*m_buffer_ptr;

    size_t const m_num_of_cells;    //!< number of memory cells allocated for the ring buffer
};

}

#endif
