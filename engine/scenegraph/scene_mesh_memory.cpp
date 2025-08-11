#include "scene_mesh_memory.h"

#include "engine/core/exception.h"
#include "engine/core/misc/misc.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/resource.h"

namespace lexgine::scenegraph
{

namespace
{

core::dx::d3d12::DedicatedUploadDataStreamAllocator createUploadStreamAllocator(core::Globals& globals, std::string const& cb_stream_section_name)
{
    core::GlobalSettings const& global_settings = *globals.get<core::GlobalSettings>();
    core::dx::d3d12::DxResourceFactory& dx_resource_factory = *globals.get<core::dx::d3d12::DxResourceFactory>();
    core::dx::d3d12::Heap& upload_heap = dx_resource_factory.retrieveUploadHeap(*globals.get<core::dx::d3d12::Device>());
    auto upload_heap_section = dx_resource_factory.allocateSectionInUploadHeap(
        upload_heap,
        cb_stream_section_name,
        global_settings.getStreamedGeometryDataPartitionSize());
    core::dx::d3d12::UploadHeapPartition upload_heap_partition{ *upload_heap_section };
    return core::dx::d3d12::DedicatedUploadDataStreamAllocator{ globals, upload_heap_partition.offset, upload_heap_partition.size };
}

}

SceneMeshMemory::SceneMeshMemory(core::Globals& globals, uint64_t size)
    : m_gpu_scene_memory_buffer{
        *globals.get<core::dx::d3d12::Device>(),
        core::dx::d3d12::ResourceState::base_values::common,
        core::misc::Optional<core::dx::d3d12::ResourceOptimizedClearValue> {},
        core::dx::d3d12::ResourceDescriptor::CreateBuffer(size, core::dx::d3d12::ResourceFlags::base_values::none),
        core::dx::d3d12::AbstractHeapType::_default,
        core::dx::d3d12::HeapCreationFlags::base_values::allow_only_buffers
    }
    , m_upload_data_stream_allocator{ createUploadStreamAllocator(globals, core::dx::d3d12::DxResourceFactory::c_dynamic_geometry_section_name) }
    , m_resource_data_uploader{ globals, m_upload_data_stream_allocator, core::dx::d3d12::ResourceUploadPolicy::non_blocking }
{

}

SceneMemoryBufferHandle SceneMeshMemory::addData(void const* p_data, size_t size)
{
    core::dx::d3d12::ResourceDataUploader::DestinationDescriptor destination_desc{
            .p_destination_resource = &m_gpu_scene_memory_buffer,
            .destination_resource_state = core::dx::d3d12::ResourceState::base_values::common
    };
    destination_desc.segment.base_offset = m_data_upload_offset;

    core::dx::d3d12::ResourceDataUploader::BufferSourceDescriptor source_desc{
            .p_data = p_data,
            .buffer_size = size
    };

    if (!m_resource_data_uploader.addResourceForUpload(destination_desc, source_desc))
    {
        uploadAllData();
        m_resource_data_uploader.waitUntilUploadIsFinished();
        assert(m_resource_data_uploader.addResourceForUpload(destination_desc, source_desc));
    }

    size_t data_offset = m_data_upload_offset;
    m_data_upload_offset += size;    // update offset for subsequent upload
    return { .offset = data_offset, .size = size };
}

void SceneMeshMemory::uploadAllData()
{
    m_resource_data_uploader.upload();
}

void SceneMeshMemory::reset()
{
    waitUntillReady();
    m_data_upload_offset = 0;
}

bool SceneMeshMemory::isReady() const
{
    return m_resource_data_uploader.isUploadFinished();
}

void SceneMeshMemory::waitUntillReady() const
{
    m_resource_data_uploader.waitUntilUploadIsFinished();
}

}
