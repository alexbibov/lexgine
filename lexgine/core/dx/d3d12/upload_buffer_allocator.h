#ifndef LEXGINE_CORE_DX_D3D12_UPLOAD_BUFFER_ALLOCATOR
#define LEXGINE_CORE_DX_D3D12_UPLOAD_BUFFER_ALLOCATOR

#include "signal.h"
#include "lexgine/core/entity.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/allocator.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/resource.h"

#include <list>
#include <mutex>


namespace lexgine::core::dx::d3d12
{

class UploadBufferBlock final
{
    friend class UploadBufferAllocator;
    friend class Allocator<UploadBufferBlock>::memory_block_type;

public:
    size_t capacity() const;
    bool isInUse() const;
    void* address() const;
    void* offsetted_address(uint32_t offset) const;
    uint32_t offset() const;  //! returns offset of the starting address of the allocation within the upload buffer

private:
    UploadBufferBlock(Signal const& tracking_signal, void* mapped_gpu_buffer_addr,
        uint64_t signal_value, uint32_t allocation_begin, uint32_t allocation_end);

private:
    Signal const& m_tracking_signal;
    unsigned char* m_mapped_gpu_buffer_addr;
    uint64_t m_controlling_signal_value;
    uint32_t m_allocation_begin;
    uint32_t m_allocation_end;
};

class UploadBufferAllocator : public Allocator<UploadBufferBlock>,
    public NamedEntity<class_names::D3D12_UploadBufferAllocator>
{
public:
    UploadBufferAllocator(Globals& globals, 
        uint64_t offset_from_heap_start, size_t upload_buffer_size);

    virtual ~UploadBufferAllocator();

    address_type allocate(size_t size_in_bytes, bool is_blocking_call = true);

    void signalAllocator(CommandQueue const& signalling_queue) const;

    uint64_t completedWork() const;
    uint64_t scheduledWork() const;
    uint64_t totalCapacity() const;

    Resource const& getUploadResource() const;

private:
    using list_of_allocations = std::list<memory_block_type>;

private:
    DxResourceFactory& m_dx_resource_factory;
    Device& m_device;
    Heap& m_upload_heap;
    Signal m_progress_tracking_signal;
    PlacedResource m_upload_buffer;

    std::mutex m_access_semaphore;
    list_of_allocations m_blocks;
    list_of_allocations::iterator m_last_allocation;
    size_t m_unpartitioned_chunk_size;
    uint32_t m_max_non_blocking_allocation_timeout;

    void* m_upload_buffer_mapping;
};

}

#endif
