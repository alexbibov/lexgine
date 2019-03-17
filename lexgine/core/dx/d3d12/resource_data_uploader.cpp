#include <cassert>

#include "resource_data_uploader.h"
#include "device.h"
#include "resource_barrier_pack.h"

#include "lexgine/core/exception.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;



ResourceDataUploader::ResourceDataUploader(Globals& globals, uint64_t offset, uint64_t capacity) :
    m_device{ *globals.get<Device>() },
    m_upload_buffer_allocator{ globals, offset, capacity },
    m_upload_commands_list{ m_device.createCommandList(CommandType::copy, 0x1) },
    m_upload_command_list_needs_reset{ true },
    m_copy_destination_resource_state{ globals.get<GlobalSettings>()->isAsyncCopyEnabled() ? ResourceState::enum_type::common : ResourceState::enum_type::copy_destination }
{

}

void ResourceDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor, SourceDescriptor const& source_descriptor)
{
    if (m_upload_command_list_needs_reset)
    {
        m_upload_commands_list.reset();
        m_upload_command_list_needs_reset = false;
    }

    {
        StaticResourceBarrierPack<1> barriers;

        barriers.addTransitionBarrier(destination_descriptor.p_destination_resource,
            destination_descriptor.destination_resource_state, m_copy_destination_resource_state);
        barriers.applyBarriers(m_upload_commands_list);
    }
    
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->descriptor().native();
    auto native_device = m_device.native();

    switch (destination_descriptor.segment_type)
    {
    case ResourceDataUploader::DestinationDescriptor::texture:
    {
        UINT64 task_size;
        uint32_t num_subresources = destination_descriptor.segment.subresources.num_subresources;
        size_t const subresource_copy_footprints_size = (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * num_subresources;
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprints = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());
        UINT* p_subresource_num_rows = reinterpret_cast<UINT*>(p_placed_subresource_footprints + num_subresources);
        UINT64* p_subresource_row_size_in_bytes = reinterpret_cast<UINT64*>(p_subresource_num_rows + num_subresources);

        native_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, 
            destination_descriptor.segment.subresources.first_subresource, destination_descriptor.segment.subresources.num_subresources,
            0U, p_placed_subresource_footprints, p_subresource_num_rows, p_subresource_row_size_in_bytes, &task_size);

        auto allocation = m_upload_buffer_allocator.allocate(task_size, false);

        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            char* p_upload_buffer_subresource_offset = static_cast<char*>(allocation->address()) + p_placed_subresource_footprints[p].Offset;
            char* p_source_subresource_offset = static_cast<char*>(source_descriptor.p_source_data) + p_placed_subresource_footprints[p].Offset;
            size_t const resource_slice_size = p_subresource_num_rows[p] * p_placed_subresource_footprints[p].Footprint.RowPitch;

            for (uint32_t k = 0; k < p_placed_subresource_footprints[p].Footprint.Depth; ++k)
            {
                char* p_upload_buffer_subresource_slice_offset = p_upload_buffer_subresource_offset + resource_slice_size*k;
                char* p_source_subresource_slice_offset = p_source_subresource_offset + source_descriptor.slice_pitch*k;

                for (uint32_t i = 0; i < p_subresource_num_rows[p]; ++i)
                {
                    char* p_upload_buffer_subresource_row_offset = p_upload_buffer_subresource_slice_offset + p_placed_subresource_footprints[p].Footprint.RowPitch*i;
                    char* p_source_subresource_row_offset = p_source_subresource_slice_offset + source_descriptor.row_pitch*i;

                    memcpy(p_upload_buffer_subresource_row_offset, p_source_subresource_row_offset, static_cast<size_t>(p_subresource_row_size_in_bytes[p]));
                }
            }
        }

        m_upload_commands_list.copyBufferRegion(*destination_descriptor.p_destination_resource, static_cast<uint64_t>(p_placed_subresource_footprints[0].Offset),
            m_upload_buffer_allocator.getUploadResource(), allocation->offset(), static_cast<uint64_t>(task_size));
       
        break;
    }

    case ResourceDataUploader::DestinationDescriptor::buffer:
    {
        size_t const subresource_copy_footprints_size = sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT);
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprint = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());

        native_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, 0, 1,
            static_cast<UINT64>(destination_descriptor.segment.base_offset), p_placed_subresource_footprint, NULL, NULL, NULL);

        auto allocation = m_upload_buffer_allocator.allocate(source_descriptor.row_pitch);
        char* p_upload_buffer_addr = static_cast<char*>(allocation->address());
        memcpy(p_upload_buffer_addr + p_placed_subresource_footprint->Offset, source_descriptor.p_source_data, static_cast<size_t>(source_descriptor.row_pitch));

        m_upload_commands_list.copyBufferRegion(*destination_descriptor.p_destination_resource, destination_descriptor.segment.base_offset,
            m_upload_buffer_allocator.getUploadResource(), allocation->offset(), source_descriptor.row_pitch);

        break;
    }

    default:
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "destination resource has unknown memory segment");
    }

    {
        StaticResourceBarrierPack<1> barriers;

        barriers.addTransitionBarrier(destination_descriptor.p_destination_resource,
            m_copy_destination_resource_state, destination_descriptor.destination_resource_state);
        barriers.applyBarriers(m_upload_commands_list);
    }
}


void ResourceDataUploader::upload()
{
    if(!m_upload_command_list_needs_reset) 
    {
        m_upload_commands_list.close(); m_upload_command_list_needs_reset = true;
        m_device.copyCommandQueue().executeCommandList(m_upload_commands_list);
        m_upload_buffer_allocator.signalAllocator(m_device.copyCommandQueue());
    }
}

void ResourceDataUploader::waitUntilUploadIsFinished() const
{
    while (!isUploadFinished()) YieldProcessor();
}

bool ResourceDataUploader::isUploadFinished() const
{
    return m_upload_buffer_allocator.scheduledWork() == m_upload_buffer_allocator.completedWork();
}
