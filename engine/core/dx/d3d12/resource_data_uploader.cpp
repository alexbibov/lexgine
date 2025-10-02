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

bool ResourceDataUploader::addResourceForUpload(
    DestinationDescriptor const& destination_descriptor,
    TextureSourceDescriptor const& source_descriptor
)
{
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->descriptor().native();
    assert(d3d12_destination_resource_descriptor.Dimension >= D3D12_RESOURCE_DIMENSION_TEXTURE1D
        && d3d12_destination_resource_descriptor.Dimension <= D3D12_RESOURCE_DIMENSION_TEXTURE3D);

    {
        uint32_t num_subresources = destination_descriptor.segment.subresources.num_subresources;
        size_t const subresource_copy_footprints_size = (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * num_subresources;
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprints = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());
        UINT* p_subresource_num_rows = reinterpret_cast<UINT*>(p_placed_subresource_footprints + num_subresources);
        UINT64* p_subresource_row_size_in_bytes = reinterpret_cast<UINT64*>(p_subresource_num_rows + num_subresources);

        assert(num_subresources <= source_descriptor.subresources.size());

        m_device.native()->GetCopyableFootprints(&d3d12_destination_resource_descriptor,
            destination_descriptor.segment.subresources.first_subresource, destination_descriptor.segment.subresources.num_subresources,
            0U, p_placed_subresource_footprints, p_subresource_num_rows, p_subresource_row_size_in_bytes, NULL);

        size_t task_size = 0;
        std::vector<DestinationDescriptor::SubresourceRegion> const& dst_regions = destination_descriptor.destination_regions;
        std::vector<size_t> allocation_buffer_offsets(dst_regions.size());
        std::vector<size_t> row_pitches(dst_regions.size());
        for (size_t p = 0; p < dst_regions.size(); ++p)
        {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint_desc = p_placed_subresource_footprints[p];
            assert(p_subresource_row_size_in_bytes[p] % footprint_desc.Footprint.Width == 0);
            uint32_t const texel_size = p_subresource_row_size_in_bytes[p] / footprint_desc.Footprint.Width;
            DestinationDescriptor::SubresourceRegion const& region = dst_regions[p];
            allocation_buffer_offsets[p] = task_size;
            row_pitches[p] = (region.width * texel_size + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
            task_size += row_pitches[p]
                * region.height
                * region.depth;
        }
        for (size_t p = dst_regions.size(); p < num_subresources; ++p)
        {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint_desc = p_placed_subresource_footprints[p];
            task_size += p_subresource_row_size_in_bytes[p]
                * footprint_desc.Footprint.Height
                * footprint_desc.Footprint.Depth;
        }

        bool is_allocation_blocking = m_upload_policy == ResourceUploadPolicy::blocking;
        auto allocation = m_upload_buffer_allocator.allocate(task_size, is_allocation_blocking);
        if (allocation == nullptr)
        {
            return false;    // staging buffer is exhausted
        }

        beginCopy(destination_descriptor);
        size_t offset = 0;
        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint_desc = p_placed_subresource_footprints[p];
            DestinationDescriptor::SubresourceRegion const* p_dst_region = nullptr;
            if (p < static_cast<uint32_t>(dst_regions.size()))
            {
                p_dst_region = &dst_regions[p];
            }

            uint8_t* p_dst_subresource = static_cast<uint8_t*>(allocation->cpuAddress()) 
                + (p_dst_region ? allocation_buffer_offsets[p] : footprint_desc.Offset);
            uint8_t const* p_src_subresource = static_cast<uint8_t const*>(source_descriptor.subresources[p].p_data);
            
            uint32_t const subresource_num_rows = p_dst_region ? p_dst_region->height : static_cast<uint32_t>(p_subresource_num_rows[p]);
            size_t const dst_subresource_slice_pitch = subresource_num_rows * (p_dst_region ? row_pitches[p] : footprint_desc.Footprint.RowPitch);
            size_t const src_subresource_slice_pitch = source_descriptor.subresources[p].slice_pitch;
            size_t const dst_subresource_row_pitch = p_dst_region ? row_pitches[p] : static_cast<size_t>(p_subresource_row_size_in_bytes[p]);
            size_t const src_subresource_row_pitch = source_descriptor.subresources[p].row_pitch;
            size_t const src_subresource_row_size = source_descriptor.subresources[p].row_size;
            uint32_t const dst_depth = p_dst_region ? p_dst_region->depth : static_cast<uint32_t>(footprint_desc.Footprint.Depth);

            assert(dst_depth > 1 && src_subresource_slice_pitch != 0 || dst_depth == 1);
            assert(src_subresource_row_pitch > 0 && src_subresource_row_size > 0);

            for (uint32_t k = 0; k < dst_depth; ++k)
            {
                uint8_t* p_dst_subresource_slice = p_dst_subresource + dst_subresource_slice_pitch * k;
                uint8_t const* p_src_subresource_slice = p_src_subresource + src_subresource_slice_pitch * k;

                for (uint32_t i = 0; i < subresource_num_rows; ++i)
                {
                    uint8_t* p_dst_subresource_row = p_dst_subresource_slice + dst_subresource_row_pitch * i;
                    uint8_t const* p_src_subresource_row = p_src_subresource_slice + src_subresource_row_pitch * i;
                    std::copy(
                        p_src_subresource_row, 
                        p_src_subresource_row + src_subresource_row_size, 
                        p_dst_subresource_row
                    );
                }
            }

            uint32_t const dst_width = p_dst_region ? p_dst_region->width : footprint_desc.Footprint.Width;
            uint32_t const dst_height = p_dst_region ? p_dst_region->height : footprint_desc.Footprint.Height;
            math::Vector3u dst_offset = p < dst_regions.size() 
                ? math::Vector3u{dst_regions[p].offset_x, dst_regions[p].offset_y, dst_regions[p].offset_z}
                : math::Vector3u{};

            TextureCopyLocation source_location{
                m_upload_buffer_allocator.getUploadResource(),
                allocation->offset() + footprint_desc.Offset,
                footprint_desc.Footprint.Format, 
                dst_width,
                dst_height,
                dst_depth,
                footprint_desc.Footprint.RowPitch 
            };

            TextureCopyLocation destination_location{ 
                *destination_descriptor.p_destination_resource,
                p
            };

            m_upload_command_list.copyTextureRegion(
                destination_location, 
                misc::Optional<math::Vector3f>{dst_offset},
                source_location, 
                misc::Optional<math::Box>{}
            );
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