#include "scene_mesh_memory.h"

#include "engine/core/exception.h"
#include "engine/core/misc/misc.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"

namespace lexgine::scenegraph
{

core::dx::d3d12::DedicatedUploadDataStreamAllocator createUploadStreamAllocator(core::Globals& globals)
{
    core::GlobalSettings const& global_settings = *globals.get<core::GlobalSettings>();
    core::dx::d3d12::DxResourceFactory& dx_resource_factory = *globals.get<core::dx::d3d12::DxResourceFactory>();
    core::dx::d3d12::Heap& upload_heap = dx_resource_factory.retrieveUploadHeap(*globals.get<core::dx::d3d12::Device>());
    auto upload_heap_section = dx_resource_factory.allocateSectionInUploadHeap(
        upload_heap, 
        core::dx::d3d12::DxResourceFactory::c_dynamic_geometry_section_name,
        global_settings.getStreamedGeometryDataPartitionSize()
    );
    core::dx::d3d12::UploadHeapPartition upload_heap_partition{ *upload_heap_section };
    return core::dx::d3d12::DedicatedUploadDataStreamAllocator{ globals, upload_heap_partition.offset, upload_heap_partition.size };
}

SceneMeshMemory::SceneMeshMemory(core::Globals& globals, uint64_t size)
    : m_globals{ globals }
    , m_gpu_scene_memory_buffer{
        *globals.get<core::dx::d3d12::Device>(), 
        core::dx::d3d12::ResourceState::base_values::common,
        core::misc::Optional<core::dx::d3d12::ResourceOptimizedClearValue>{},
        core::dx::d3d12::ResourceDescriptor::CreateBuffer(size, core::dx::d3d12::ResourceFlags::base_values::none),
        core::dx::d3d12::AbstractHeapType::_default,
        core::dx::d3d12::HeapCreationFlags::base_values::allow_only_buffers
    }
    , m_upload_data_stream_allocator{ createUploadStreamAllocator(globals) }
    , m_resource_data_uploader{ globals, m_upload_data_stream_allocator }
{

}

MeshBufferHandle SceneMeshMemory::addData(void const* p_data, size_t size)
{
    std::lock_guard<std::mutex> lock { m_data_upload_mutex };

    bool start_uploading_thread = m_upload_request_queue.empty();
    size_t data_offset = m_data_upload_offset;
    m_upload_request_queue.push(DataUploadRequestDesc{ .p_source_data = p_data, .data_size = size, .upload_offset = data_offset });
    m_data_upload_offset += size;

    if (start_uploading_thread)
    {
        m_data_upload_worker = std::thread{ &SceneMeshMemory::dataUploadWatchdog, this };
        HANDLE native_thread_handle = m_data_upload_worker.native_handle();
        SetThreadDescription(native_thread_handle, L"Scene memory mesh data upload thread");
    }

    return { .offset = data_offset, .size = size };
}

bool SceneMeshMemory::isReady() const
{
    std::lock_guard<std::mutex> lock{ m_data_upload_mutex };
    return m_upload_request_queue.empty() && m_resource_data_uploader.isUploadFinished();
}

void SceneMeshMemory::dataUploadWatchdog()
{
    if (!m_resource_data_uploader.isUploadFinished())
    {
        m_resource_data_uploader.waitUntilUploadIsFinished();
    }

    while (!m_upload_request_queue.empty())
    {
        DataUploadRequestDesc desc{};
        {
            std::lock_guard<std::mutex> lock{ m_data_upload_mutex };
            desc = m_upload_request_queue.front();
            m_upload_request_queue.pop();
        }

        core::dx::d3d12::ResourceDataUploader::DestinationDescriptor destination_desc{
            .p_destination_resource = &m_gpu_scene_memory_buffer, 
            .destination_resource_state = core::dx::d3d12::ResourceState::base_values::common
        };
        destination_desc.segment.base_offset = desc.upload_offset;
        core::dx::d3d12::ResourceDataUploader::BufferSourceDescriptor source_desc{
            .p_data = const_cast<void*>(desc.p_source_data),
            .buffer_size = desc.data_size
        };
        if (!m_resource_data_uploader.addResourceForUpload(destination_desc, source_desc))
        {
            m_resource_data_uploader.upload();
            m_resource_data_uploader.waitUntilUploadIsFinished();
            assert(m_resource_data_uploader.addResourceForUpload(destination_desc, source_desc));
        }
    }
    m_resource_data_uploader.upload();
}


}
