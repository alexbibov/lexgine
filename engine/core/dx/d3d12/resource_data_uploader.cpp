#include <cassert>
#include <thread>

#include "resource_data_uploader.h"
#include "device.h"
#include "resource_barrier_pack.h"

#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"

#include "engine/core/math/box.h"

namespace lexgine::core::dx::d3d12 {

ResourceDataUploader::ResourceDataUploader(Globals& globals, DedicatedUploadDataStreamAllocator& upload_buffer_allocator, ResourceUploadPolicy upload_policy)
    : m_device{ *globals.get<Device>() }
    , m_is_async_copy_enabled{ globals.get<GlobalSettings>()->isAsyncCopyEnabled() }
    , m_upload_buffer_allocator{ upload_buffer_allocator }
    , m_upload_command_list{ m_device.createCommandList(m_is_async_copy_enabled ? CommandType::copy : CommandType::direct, 0x1) }
    , m_upload_policy{ upload_policy }
{

}

bool ResourceDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor,
    TextureSourceDescriptor const& source_descriptor)
{
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->descriptor().native();
    assert(d3d12_destination_resource_descriptor.Dimension >= D3D12_RESOURCE_DIMENSION_TEXTURE1D
        && d3d12_destination_resource_descriptor.Dimension <= D3D12_RESOURCE_DIMENSION_TEXTURE3D);

    {
        UINT64 task_size;
        uint32_t num_subresources = destination_descriptor.segment.subresources.num_subresources;
        size_t const subresource_copy_footprints_size = (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * num_subresources;
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprints = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());
        UINT* p_subresource_num_rows = reinterpret_cast<UINT*>(p_placed_subresource_footprints + num_subresources);
        UINT64* p_subresource_row_size_in_bytes = reinterpret_cast<UINT64*>(p_subresource_num_rows + num_subresources);

        assert(num_subresources <= source_descriptor.subresources.size());

        m_device.native()->GetCopyableFootprints(&d3d12_destination_resource_descriptor,
            destination_descriptor.segment.subresources.first_subresource, destination_descriptor.segment.subresources.num_subresources,
            0U, p_placed_subresource_footprints, p_subresource_num_rows, p_subresource_row_size_in_bytes, &task_size);

        bool is_allocation_blocking = m_upload_policy == ResourceUploadPolicy::blocking;
        auto allocation = m_upload_buffer_allocator.allocate(task_size, is_allocation_blocking);
        if (allocation == nullptr)
        {
            return false;    // staging buffer is exhausted
        }

        beginCopy(destination_descriptor);
        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint_desc = p_placed_subresource_footprints[p];
            assert(footprint_desc.Offset == 0);
            uint8_t* p_dst_subresource = static_cast<uint8_t*>(allocation->cpuAddress());
            uint8_t const* p_src_subresource = static_cast<uint8_t const*>(source_descriptor.subresources[p].p_data);

            size_t const dst_subresource_slice_pitch = p_subresource_num_rows[p] * footprint_desc.Footprint.RowPitch;
            size_t const src_subresource_slice_pitch = source_descriptor.subresources[p].slice_pitch;
            size_t const src_subresource_row_pitch = source_descriptor.subresources[p].row_pitch;

            for (uint32_t k = 0; k < footprint_desc.Footprint.Depth; ++k)
            {
                uint8_t* p_dst_subresource_slice = p_dst_subresource + dst_subresource_slice_pitch * k;
                uint8_t const* p_src_subresource_slice = p_src_subresource + src_subresource_slice_pitch * k;

                for (uint32_t i = 0; i < p_subresource_num_rows[p]; ++i)
                {
                    uint8_t* p_dst_subresource_row = p_dst_subresource_slice + p_subresource_row_size_in_bytes[p] * i;
                    uint8_t const* p_src_subresource_row = p_src_subresource_slice + src_subresource_row_pitch * i;
                    std::copy(
                        p_src_subresource_row, 
                        p_src_subresource_row + src_subresource_row_pitch, 
                        p_dst_subresource_row
                    );
                }
            }

            TextureCopyLocation source_location{ m_upload_buffer_allocator.getUploadResource(),
                allocation->offset(),
                footprint_desc.Footprint.Format, footprint_desc.Footprint.Width,
                footprint_desc.Footprint.Height, footprint_desc.Footprint.Depth,
                footprint_desc.Footprint.RowPitch };

            TextureCopyLocation destination_location{ *destination_descriptor.p_destination_resource, p };

            m_upload_command_list.copyTextureRegion(destination_location, misc::Optional<math::Vector3u>{},
                source_location, misc::Optional<math::Box>{});
        }
        endCopy(destination_descriptor);
    }
    return true;
}

bool ResourceDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor,
    BufferSourceDescriptor const& source_descriptor)
{
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->descriptor().native();
    assert(d3d12_destination_resource_descriptor.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

    {
        size_t const subresource_copy_footprints_size = sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT);
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprint = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());

        m_device.native()->GetCopyableFootprints(&d3d12_destination_resource_descriptor, 0, 1,
            static_cast<UINT64>(destination_descriptor.segment.base_offset), p_placed_subresource_footprint, NULL, NULL, NULL);

        auto allocation = m_upload_buffer_allocator.allocate(source_descriptor.buffer_size);
        if (allocation == nullptr)
        {
            return false;    // staging buffer is exhausted
        }

        beginCopy(destination_descriptor);
        uint8_t* p_upload_buffer_addr = static_cast<uint8_t*>(allocation->cpuAddress());
        std::copy(
            static_cast<uint8_t const*>(source_descriptor.p_data),
            static_cast<uint8_t const*>(source_descriptor.p_data) + source_descriptor.buffer_size,
            p_upload_buffer_addr
        );

        m_upload_command_list.copyBufferRegion(*destination_descriptor.p_destination_resource, destination_descriptor.segment.base_offset,
            m_upload_buffer_allocator.getUploadResource(), allocation->offset(), source_descriptor.buffer_size);
        endCopy(destination_descriptor);
    }
    return true;
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
        std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
        std::this_thread::yield();
    }
}

bool ResourceDataUploader::isUploadFinished() const
{
    return m_upload_buffer_allocator.scheduledWork() == m_upload_buffer_allocator.completedWork();
}

uint64_t ResourceDataUploader::availableCapacity() const
{
    return m_upload_buffer_allocator.getFragmentationCapacity() + m_upload_buffer_allocator.getUnpartitionedCapacity();
}

void ResourceDataUploader::beginCopy(DestinationDescriptor const& destination_descriptor)
{
    if (m_upload_command_list_needs_reset)
    {
        m_upload_command_list.reset();
        m_upload_command_list_needs_reset = false;
    }

    if (m_is_async_copy_enabled) 
    {
        return;
    }

    // begin-of-copy barrier

    StaticResourceBarrierPack<1> barriers;

    barriers.addTransitionBarrier(destination_descriptor.p_destination_resource,
        destination_descriptor.destination_resource_state, ResourceState::base_values::copy_destination);
    barriers.applyBarriers(m_upload_command_list);
}

void ResourceDataUploader::endCopy(DestinationDescriptor const& destination_descriptor)
{
    if (m_is_async_copy_enabled) 
    {
        return;
    }

    // end-of-copy barrier

    StaticResourceBarrierPack<1> barriers;

    barriers.addTransitionBarrier(destination_descriptor.p_destination_resource,
        ResourceState::base_values::copy_destination, destination_descriptor.destination_resource_state);
    barriers.applyBarriers(m_upload_command_list);
}

}