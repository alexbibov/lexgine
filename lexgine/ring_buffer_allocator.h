#ifndef LEXGINE_CORE_RING_BUFFER_H

#include <unordered_map>

namespace lexgine {namespace core {

/*! Ring buffer allocation manager. Note that ring buffer does not support
 constructors or destructors for type T. Therefore, it has to be used for primitive types only.
*/
template<typename T>
class RingBufferAllocator
{
public:
    //! Initializes ring buffer with given number of memory cells
    RingBufferAllocator(size_t number_of_cells = 128U) :
        m_num_of_cells{ number_of_cells }
    {
        if (number_of_cells)
        {
            p_buf = new RingBufferCell{ false, nullptr };
            m_addr_cell_to_internal.insert(std::make_pair(reinterpret_cast<size_t>(&p_buf->cell_data), reinterpret_cast<size_t>(p_buf)));
            --number_of_cells;

            RingBufferCell* p_current_cell = p_buf;
            for (size_t i = 0; i < number_of_cells; ++i)
            {
                p_current_cell->p_next = new RingBufferCell{ false, nullptr };
                p_current_cell = p_current_cell->p_next;
                m_addr_cell_to_internal.insert(std::make_pair(reinterpret_cast<size_t>(&p_current_cell->cell_data), reinterpret_cast<size_t>(p_current_cell)));
            }
            p_current_cell->p_next = p_buf;
        }
    }

    ~RingBufferAllocator()
    {
        RingBufferCell* p_current_cell = p_buf;
        for (size_t i = 0; i < m_num_of_cells; ++i)
        {
            RingBufferCell* p_next = p_current_cell->p_next;
            delete p_current_cell;
            p_current_cell = p_next;
        }
    }

    //! Allocates new object of type T from the ring buffer
    T* allocate()
    {
        RingBufferCell* p_current_cell = p_buf;
        uint32_t num_parsed_count{ 0U };
        while (num_parsed_count < m_num_of_cells && p_current_cell->is_used)
        {
            p_current_cell = p_current_cell->p_next;
            ++num_parsed_count;
        }

        if (num_parsed_count == m_num_of_cells)
            return nullptr;    // the ring buffer is exhausted

        p_current_cell->is_used = true;
        return &p_current_cell->cell_data;
    }


    /*! Removes object having provided address from the ring buffer. If the input address does not point to a valid
     object of type T currently residing in the ring buffer the behavior is undefined.
    */
    void free(T* p_instance)
    {
        reinterpret_cast<RingBufferCell*>(m_addr_cell_to_internal[reinterpret_cast<size_t>(p_instance)])->is_used = false;
    }

private:
    struct RingBufferCell
    {
        T cell_data;    //!< data stored in the ring buffer cell
        bool is_used;    //!< 'true' if the cell is currently in use, 'false' otherwise
        RingBufferCell* p_next;    //!< pointer to the next cell in the ring buffer

        RingBufferCell(bool is_used, RingBufferCell* p_next) :
            is_used{ is_used },
            p_next{ p_next }
        {

        }

    }*p_buf;

    size_t const m_num_of_cells;    //!< number of memory cells allocated for the ring buffer
    std::unordered_map<size_t, size_t> m_addr_cell_to_internal;    //!< maps address of cell_data member to the address of the ring buffer cell, which contains this cell_data
};

}}

#define LEXGINE_CORE_RING_BUFFER_H
#endif
