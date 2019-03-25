#include <cassert>

#include "resource_data_uploader.h"
#include "device.h"
#include "resource_barrier_pack.h"

#include "lexgine/core/exception.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;


ResourceDataUploader::ResourceDataUploader(Globals& globals, uint64_t offset_in_upload_heap, size_t upload_section_size)
    : m_device{ *globals.get<Device>() }
    , m_is_async_copy_enabled{ globals.get<GlobalSettings>()->isAsyncCopyEnabled() }
    , m_upload_buffer_allocator{ globals, offset_in_upload_heap, upload_section_size }
    , m_upload_command_list{ m_device.createCommandList(m_is_async_copy_enabled ? CommandType::copy : CommandType::direct, 0x1) }
    , m_upload_command_list_needs_reset{ true }
    , m_copy_destination_resource_state{ m_is_async_copy_enabled ? ResourceState::enum_type::common : ResourceState::enum_type::copy_destination }
{

}

void ResourceDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor,
    TextureSourceDescriptor const& source_descriptor)
{
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->descriptor().native();
    assert(d3d12_destination_resource_descriptor.Dimension >= D3D12_RESOURCE_DIMENSION_TEXTURE1D
        || d3d12_destination_resource_descriptor.Dimension <= D3D12_RESOURCE_DIMENSION_TEXTURE3D);

    beginCopy(destination_descriptor);
    {
        UINT64 task_size;
        uint32_t num_subresources = destination_descriptor.segment.subresources.num_subresources;
        size_t const subresource_copy_footprints_size = (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * num_subresources;
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprints = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());
        UINT* p_subresource_num_rows = reinterpret_cast<UINT*>(p_placed_subresource_footprints + num_subresources);
        UINT64* p_subresource_row_size_in_bytes = reinterpret_cast<UINT64*>(p_subresource_num_rows + num_subresources);

        assert(num_subresources == source_descriptor.subresources.size());

        m_device.native()->GetCopyableFootprints(&d3d12_destination_resource_descriptor,
            destination_descriptor.segment.subresources.first_subresource, destination_descriptor.segment.subresources.num_subresources,
            0U, p_placed_subresource_footprints, p_subresource_num_rows, p_subresource_row_size_in_bytes, &task_size);

        auto allocation = m_upload_buffer_allocator.allocate(task_size);
        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            char* p_dst_subresource = static_cast<char*>(allocation->address()) + p_placed_subresource_footprints[p].Offset;
            char* p_src_subresource = static_cast<char*>(source_descriptor.subresources[p].p_data);

            size_t const dst_subresource_slice_pitch = p_subresource_num_rows[p] * p_placed_subresource_footprints[p].Footprint.RowPitch;
            size_t const src_subresource_slice_pitch = source_descriptor.subresources[p].slice_pitch;
            size_t const src_subresource_row_pitch = source_descriptor.subresources[p].row_pitch;

            for (uint32_t k = 0; k < p_placed_subresource_footprints[p].Footprint.Depth; ++k)
            {
                char* p_dst_subresource_slice = p_dst_subresource + dst_subresource_slice_pitch * k;
                char* p_src_subresource_slice = p_src_subresource + src_subresource_slice_pitch * k;

                for (uint32_t i = 0; i < p_subresource_num_rows[p]; ++i)
                {
                    char* p_dst_subresource_row = p_dst_subresource_slice + p_subresource_row_size_in_bytes[p] * i;
                    char* p_src_subresource_row = p_src_subresource_slice + src_subresource_row_pitch * i;
                    memcpy(p_dst_subresource_row, p_src_subresource_row, src_subresource_row_pitch);
                }
            }
        }

        m_upload_command_list.copyBufferRegion(*destination_descriptor.p_destination_resource, static_cast<uint64_t>(p_placed_subresource_footprints[0].Offset),
            m_upload_buffer_allocator.getUploadResource(), allocation->offset(), static_cast<uint64_t>(task_size));
    }
    endCopy(destination_descriptor);
}

void ResourceDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor,
    BufferSourceDescriptor const& source_descriptor)
{
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->descriptor().native();
    assert(d3d12_destination_resource_descriptor.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

    beginCopy(destination_descriptor);
    {
        size_t const subresource_copy_footprints_size = sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT);
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprint = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());

        m_device.native()->GetCopyableFootprints(&d3d12_destination_resource_descriptor, 0, 1,
            static_cast<UINT64>(destination_descriptor.segment.base_offset), p_placed_subresource_footprint, NULL, NULL, NULL);

        auto allocation = m_upload_buffer_allocator.allocate(source_descriptor.buffer_size);
        char* p_upload_buffer_addr = static_cast<char*>(allocation->address());
        memcpy(p_upload_buffer_addr + p_placed_subresource_footprint->Offset, source_descriptor.p_data, source_descriptor.buffer_size);

        m_upload_command_list.copyBufferRegion(*destination_descriptor.p_destination_resource, destination_descriptor.segment.base_offset,
            m_upload_buffer_allocator.getUploadResource(), allocation->offset(), source_descriptor.buffer_size);
    }
    endCopy(destination_descriptor);
}


void ResourceDataUploader::upload()
{
    if (!m_upload_command_list_needs_reset)
    {
        m_upload_command_list.close(); m_upload_command_list_needs_reset = true;
        m_device.copyCommandQueue().executeCommandList(m_upload_command_list);
        m_upload_buffer_allocator.signalAllocator(m_is_async_copy_enabled ? m_device.copyCommandQueue() : m_device.defaultCommandQueue());
    }
}

Resource const& ResourceDataUploader::sourceBuffer() const
{
    return m_upload_buffer_allocator.getUploadResource();
}

void ResourceDataUploader::waitUntilUploadIsFinished() const
{
    while (!isUploadFinished())
    {
        Sleep(1);
        YieldProcessor();
    }
}

bool ResourceDataUploader::isUploadFinished() const
{
    return m_upload_buffer_allocator.scheduledWork() == m_upload_buffer_allocator.completedWork();
}

void ResourceDataUploader::beginCopy(DestinationDescriptor const& destination_descriptor)
{
    if (m_upload_command_list_needs_reset)
    {
        m_upload_command_list.reset();
        m_upload_command_list_needs_reset = false;
    }


    // begin-of-copy barrier

    StaticResourceBarrierPack<1> barriers;

    barriers.addTransitionBarrier(destination_descriptor.p_destination_resource,
        destination_descriptor.destination_resource_state, m_copy_destination_resource_state);
    barriers.applyBarriers(m_upload_command_list);
}

void ResourceDataUploader::endCopy(DestinationDescriptor const& destination_descriptor)
{
    // end-of-copy barrier

    StaticResourceBarrierPack<1> barriers;

    barriers.addTransitionBarrier(destination_descriptor.p_destination_resource,
        m_copy_destination_resource_state, destination_descriptor.destination_resource_state);
    barriers.applyBarriers(m_upload_command_list);
}
