#ifndef LEXGINE_CORE_RING_BUFFER_H

#include "allocator.h"

namespace lexgine {namespace core {

//! Ring buffer allocation manager
template<typename T>
class RingBuffer : public Allocator
{
private:
    struct RingBufferCell
    {
        T cell_data;    //!< data stored in the ring buffer cell
        bool is_used;    //!< 'true' if the cell is currently in use, 'false' otherwise
        RingBufferCell* p_next;    //!< pointer to the next cell in the ring buffer
    };

protected:
    void* performAllocation(size_t size) override;
    bool performDeallocation(void* mem_block) override;

public:
    //! Initializes ring buffer with given number of memory cells
    RingBuffer(size_t number_of_cells);

    size_t getMemoryBlockSize(void* mem_block) const override;

};

}}

#define LEXGINE_CORE_RING_BUFFER_H
#endif
