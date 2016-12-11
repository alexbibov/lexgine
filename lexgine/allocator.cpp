#include "allocator.h"

using namespace lexgine::core;

Allocator::Allocator(size_t max_memory_available):
    m_max_available_memory{ max_memory_available },
    m_used_memory{ 0U }
{
}

void* lexgine::core::Allocator::allocate(size_t size)
{
    if (size > m_max_available_memory - m_used_memory)
        return nullptr;

    m_used_memory += size;
    return performAllocation(size);
}

bool Allocator::free(void* mem_block)
{
    size_t memory_to_free = getMemoryBlockSize(mem_block);
    bool is_freed = performDeallocation(mem_block);
    if (is_freed)
    {
        m_used_memory -= memory_to_free;
        return true;
    }

    return false;
}

size_t Allocator::getTotalMemory() const
{
    return m_max_available_memory;
}

size_t Allocator::getFreeMemory() const
{
    return m_max_available_memory - m_used_memory;
}

size_t Allocator::getUsedMemory() const
{
    return m_used_memory;
}

