#ifndef LEXGINE_CORE_DX_D3D12_UPLOAD_BUFFER_ALLOCATOR
#define LEXGINE_CORE_DX_D3D12_UPLOAD_BUFFER_ALLOCATOR

#include <list>
#include <mutex>

#include "engine/core/entity.h"
#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/allocator.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/signal.h"
#include "engine/core/misc/static_vector.h"


namespace lexgine::core::dx::d3d12
{

class UploadDataAllocator;

class UploadDataBlock final
{
    friend class UploadDataAllocator;
    friend class Allocator<UploadDataBlock>::memory_block_type;
    using memory_block_type = Allocator<UploadDataBlock>::memory_block_type;

public:
    size_t capacity() const;
    bool isInUse() const;
    void* cpuAddress() const;
    void* offsettedCpuAddress(uint32_t offset) const;
    uint64_t virtualGpuAddress() const;
    uint64_t offsettedVirtualGpuAddress(uint32_t offset) const;
    uint32_t offset() const;  //! returns offset of the starting address of the allocation within the upload buffer

private:
    UploadDataBlock(UploadDataAllocator const& allocator,
        void* buffer_cpu_addr, uint64_t buffer_virtual_gpu_addr,
        uint64_t signal_value, uint32_t allocation_begin, uint32_t allocation_end);

private:
    UploadDataAllocator const& m_allocator;
    unsigned char* m_buffer_cpu_addr;
    uint64_t m_buffer_virtual_gpu_addr;
    uint64_t m_controlling_signal_value;
    uint32_t m_allocation_begin;
    uint32_t m_allocation_end;
};

class UploadDataAllocator : public Allocator<UploadDataBlock>,
    public NamedEntity<class_names::D3D12_UploadBufferAllocator>
{
public:
    using address_type = t_address_type<memory_block_type*>;

public:
    UploadDataAllocator(Globals& globals,
        uint64_t offset_from_heap_start, size_t upload_buffer_size);

    virtual ~UploadDataAllocator();

    address_type allocate(size_t size_in_bytes, bool is_blocking_call = true);

    uint64_t completedWork() const;
    uint64_t scheduledWork() const;
    uint64_t recordingWork() const;
    uint64_t totalCapacity() const { return m_buffer_size; }

    size_t getPartitionsCount() const;
    uint64_t getUnpartitionedCapacity() const { return m_unpartitioned_chunk_size; }
    uint64_t getFragmentationCapacity() const { return m_fragmentation_capacity; }

    Resource const& getUploadResource() const;

private:
    using allocation_bucket = misc::StaticVector<memory_block_type, 200>;
    using list_of_allocation_buckets = std::list<allocation_bucket>;
    
    struct allocation_iterator
    {
        list_of_allocation_buckets::iterator bucket_iter;
        allocation_bucket::iterator block_iter;

        memory_block_type& block() { return *block_iter; }
    };

private:
    virtual void waitUntilControllingSignalValue(uint64_t value) const = 0;
    virtual bool waitUntilControllingSignalValue(uint64_t value, uint32_t timeout_in_milliseconds) const = 0;
    virtual uint64_t nextValueOfControllingSignal() const = 0;
    virtual uint64_t lastSignaledValueOfControllingSignal() const = 0;
    virtual uint64_t lastRecordedValueOfControllingSignal() const = 0;

    address_type allocateInternal(size_t size_in_bytes, bool is_blocking_call);
    memory_block_type& allocateNewMemoryBlock(uint32_t allocation_begin_address, uint32_t allocation_end_address);

private:
    Heap const& m_upload_heap;
    PlacedResource m_upload_buffer;
    uint32_t const m_max_non_blocking_allocation_timeout;

    uint64_t const m_buffer_size{};
    uint64_t m_unpartitioned_chunk_size;
    uint64_t m_fragmentation_capacity{ 0 };

    void* m_upload_buffer_cpu_address;
    uint64_t m_upload_buffer_gpu_virtual_address;

    list_of_allocation_buckets m_blocks;
    allocation_iterator m_allocation_hint;

    std::mutex m_access_semaphore;
};


class DedicatedUploadDataStreamAllocator : public UploadDataAllocator
{
public:
    DedicatedUploadDataStreamAllocator(Globals& globals,
        uint64_t offset_from_heap_start, size_t upload_buffer_size);

    void signalAllocator(CommandQueue const& signalling_queue);

private:
    void waitUntilControllingSignalValue(uint64_t value) const override;
    bool waitUntilControllingSignalValue(uint64_t value, uint32_t timeout_in_milliseconds) const override;
    uint64_t nextValueOfControllingSignal() const override;
    uint64_t lastSignaledValueOfControllingSignal() const override;
    uint64_t lastRecordedValueOfControllingSignal() const override;

private:
    Signal m_progress_tracking_signal;
};

class PerFrameUploadDataStreamAllocator : public UploadDataAllocator
{
public:
    PerFrameUploadDataStreamAllocator(Globals& globals,
        uint64_t offset_from_heap_start, uint64_t upload_buffer_size,
        FrameProgressTracker const& frame_progress_tracker);

    FrameProgressTracker const& frameProgressTracker() const;

private:
    void waitUntilControllingSignalValue(uint64_t value) const override;
    bool waitUntilControllingSignalValue(uint64_t value, uint32_t timeout_in_milliseconds) const override;
    uint64_t nextValueOfControllingSignal() const override;
    uint64_t lastSignaledValueOfControllingSignal() const override;
    uint64_t lastRecordedValueOfControllingSignal() const override;

private:
    FrameProgressTracker const& m_frame_progress_tracker;
};


}

#endif
