#include "buffer.h"

#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"

namespace lexgine::scenegraph
{

core::dx::d3d12::DedicatedUploadDataStreamAllocator createUploadStreamAllocator(core::Globals& globals)
{
    core::GlobalSettings const& global_settings = *globals.get<core::GlobalSettings>();
    core::dx::d3d12::DxResourceFactory& dx_resource_factory = *globals.get<core::dx::d3d12::DxResourceFactory>();
    core::dx::d3d12::Heap& upload_heap = dx_resource_factory.retrieveUploadHeap(*globals.get<core::dx::d3d12::Device>());
    auto upload_heap_section = dx_resource_factory.allocateSectionInUploadHeap(upload_heap, core::dx::d3d12::DxResourceFactory::c_dynamic_geometry_section_name, global_settings.getStreamedGeometryDataPartitionSize());
    
    core::dx::d3d12::UploadHeapPartition upload_heap_partition{ *upload_heap_section };
    
    return core::dx::d3d12::DedicatedUploadDataStreamAllocator{ globals, upload_heap_partition.offset, upload_heap_partition.size };
}

Buffer::Buffer(core::Globals& globals, uint64_t size)
    : m_globals{ globals }
    , m_buffer_gpu_memory_heap{ globals.get<core::dx::d3d12::Device>()->createHeap(core::dx::d3d12::AbstractHeapType::_default, size, core::dx::d3d12::HeapCreationFlags::base_values::allow_only_buffers) }
    , m_upload_data_stream_allocator{ createUploadStreamAllocator(globals) }
    , m_resource_data_uploader{ globals, m_upload_data_stream_allocator }
{

}

uint64_t Buffer::fill(void* p_src_data, size_t src_data_size_in_bytes)
{
    return 0;
}

void Buffer::startWatchdog()
{

}

void Buffer::watchDelayedUploads()
{
    while (m_intermediate_buffer.size())
    {

        std::this_thread::yield();
    }
}

}
