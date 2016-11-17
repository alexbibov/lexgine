#include <cassert>

#include "heap_data_uploader.h"
#include "device.h"
#include "exception.h"
#include "resource_barrier.h"

using namespace lexgine::core::dx::d3d12;



HeapDataUploader::HeapDataUploader(Heap& upload_heap, uint64_t offset, uint64_t size) :
    m_heap{ upload_heap },
    m_offset{ offset },
    m_size{ size },
    m_transaction_size{ 0U },
    m_upload_buffer{ upload_heap, offset, ResourceState::enum_type::generic_read, D3D12_CLEAR_VALUE{}, ResourceDescriptor::CreateBuffer(size, ResourceFlags::enum_type::none) }
{

}

void HeapDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor, SourceDescriptor const& source_descriptor)
{
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->native()->GetDesc();
    D3D12_RESOURCE_DESC d3d12_upload_resource_descriptor = m_upload_buffer.native()->GetDesc();

    auto upload_buffer_native_reference = m_upload_buffer.native();

    // Get reference to the device interface owning the upload buffer
    ID3D12Device* p_device{ nullptr };
    LEXGINE_ERROR_LOG(
        this,
        upload_buffer_native_reference->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&p_device)),
        S_OK
    );


    // Retrieve copyable footprints for the upload task
    switch (destination_descriptor.segment_type)
    {
    case HeapDataUploader::DestinationDescriptor::texture:
    {
        UINT64 task_size;
        uint32_t num_subresources = destination_descriptor.segment.subresources.num_subresources;
        size_t const subresource_copy_footprints_size = (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * num_subresources;
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprints = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());
        UINT* p_subresource_num_rows = reinterpret_cast<UINT*>(p_placed_subresource_footprints + num_subresources);
        UINT64* p_subresource_row_size_in_bytes = reinterpret_cast<UINT64*>(p_subresource_num_rows + num_subresources);

        p_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, destination_descriptor.segment.subresources.first_subresource, destination_descriptor.segment.subresources.num_subresources,
            m_transaction_size, p_placed_subresource_footprints, p_subresource_num_rows, p_subresource_row_size_in_bytes, &task_size);


        assert(m_transaction_size + task_size <= d3d12_upload_resource_descriptor.Width);


        // Copy contents of the current upload task into upload buffer
        char* p_upload_buffer_addr{ nullptr };
        LEXGINE_ERROR_LOG(
            this,
            upload_buffer_native_reference->Map(0, NULL, reinterpret_cast<void**>(&p_upload_buffer_addr)),
            S_OK
        );
        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            char* p_upload_buffer_subresource_offset = p_upload_buffer_addr + p_placed_subresource_footprints[p].Offset;
            char* p_source_subresource_offset = static_cast<char*>(source_descriptor.p_source_data) + p_placed_subresource_footprints[p].Offset - m_transaction_size;
            size_t const resource_slice_size = p_subresource_num_rows[p] * p_placed_subresource_footprints[p].Footprint.RowPitch;

            for (uint32_t k = 0; k < p_placed_subresource_footprints[p].Footprint.Depth; ++k)
            {
                char* p_upload_buffer_subresource_slice_offset = p_upload_buffer_subresource_offset + resource_slice_size*k;
                char* p_source_subresource_slice_offset = p_source_subresource_offset + source_descriptor.slice_pitch*k;

                for (uint32_t i = 0; i < p_subresource_num_rows[p]; ++i)
                {
                    char* p_upload_buffer_subresource_row_offset = p_upload_buffer_subresource_slice_offset + p_placed_subresource_footprints[p].Footprint.RowPitch*i;
                    char* p_source_subresource_row_offset = p_source_subresource_slice_offset + source_descriptor.row_pitch*i;

                    memcpy(p_upload_buffer_subresource_row_offset, p_source_subresource_row_offset, p_subresource_row_size_in_bytes[p]);
                }
            }
        }
        upload_buffer_native_reference->Unmap(0, NULL);

        m_upload_tasks.emplace_back(source_descriptor, destination_descriptor, placed_subresource_footprints_buffer);

        m_transaction_size += task_size;
        break;
    }

    case HeapDataUploader::DestinationDescriptor::buffer:
    {
        size_t const subresource_copy_footprints_size = sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT);
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprint = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());

        p_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, 0, 1, m_transaction_size, p_placed_subresource_footprint, NULL, NULL, NULL);

        assert(m_transaction_size + source_descriptor.row_pitch <= d3d12_upload_resource_descriptor.Width);

        // Copy the contents of the current upload task into upload buffer
        char* p_upload_buffer_addr{ nullptr };
        LEXGINE_ERROR_LOG(
            this,
            upload_buffer_native_reference->Map(0, NULL, reinterpret_cast<void**>(&p_upload_buffer_addr)),
            S_OK
        );
        memcpy(p_upload_buffer_addr + p_placed_subresource_footprint->Offset, source_descriptor.p_source_data, source_descriptor.row_pitch);
        upload_buffer_native_reference->Unmap(0, NULL);

        m_upload_tasks.emplace_back(source_descriptor, destination_descriptor, placed_subresource_footprints_buffer);

        m_transaction_size += source_descriptor.row_pitch;
        break;
    }

    default:
        throw lexgine::core::Exception{ *this, "destination resource is having unknown memory segment" };
    }
}


void HeapDataUploader::upload(CommandList& upload_worker_list)
{
    // Apply resource transition barrier to the upload buffer
    ResourceState upload_buffer_target_state, destination_resource_target_state;
    switch (upload_worker_list.type())
    {
    case CommandListType::direct:
    case CommandListType::compute:
    case CommandListType::bundle:
        upload_buffer_target_state = ResourceState::enum_type::copy_source;
        destination_resource_target_state = ResourceState::enum_type::copy_destination;
        break;

    case CommandListType::copy:
        upload_buffer_target_state = destination_resource_target_state = ResourceState::enum_type::common;
        break;
    }


    ResourceBarrier<1> upload_buffer_transfer_barrier{ upload_worker_list };
    upload_buffer_transfer_barrier.addTransitionBarrier(&m_upload_buffer, upload_buffer_target_state, SplitResourceBarrierFlags::none);
    upload_buffer_transfer_barrier.applyBarriers();


    for (auto& task : m_upload_tasks)
    {
        ResourceState current_destination_resource_state = task.destination_descriptor.p_destination_resource->getCurrentState();


        {
            ResourceBarrier<1> destination_resource_transfer_barrier{ upload_worker_list };
            destination_resource_transfer_barrier.addTransitionBarrier(task.destination_descriptor.p_destination_resource, destination_resource_target_state, SplitResourceBarrierFlags::none);
            destination_resource_transfer_barrier.applyBarriers();
        }

        switch (task.destination_descriptor.segment_type)
        {
        case HeapDataUploader::DestinationDescriptor::texture:
        {
            D3D12_TEXTURE_COPY_LOCATION dst_desc;
            D3D12_TEXTURE_COPY_LOCATION src_desc;

            for (uint32_t p = task.destination_descriptor.segment.subresources.first_subresource;
                p < task.destination_descriptor.segment.subresources.first_subresource + task.destination_descriptor.segment.subresources.num_subresources;
                ++p)
            {
                dst_desc.pResource = task.destination_descriptor.p_destination_resource->native().Get();
                dst_desc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dst_desc.SubresourceIndex = p;

                src_desc.pResource = m_upload_buffer.native().Get();
                src_desc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                src_desc.PlacedFootprint = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(task.subresource_footprints_buffer.data())[p];

                upload_worker_list.native()->CopyTextureRegion(&dst_desc, 0, 0, 0, &src_desc, NULL);
            }
            break;
        }

        case HeapDataUploader::DestinationDescriptor::buffer:
            upload_worker_list.native()->CopyBufferRegion(task.destination_descriptor.p_destination_resource->native().Get(), task.destination_descriptor.segment.base_offset,
                m_upload_buffer.native().Get(), static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(task.subresource_footprints_buffer.data())->Offset, task.source_descriptor.row_pitch);
            break;

        default:
            throw lexgine::core::Exception{ *this, "destination resource is having unknown memory segment" };
        }


        {
            ResourceBarrier<1> destination_resource_transfer_barrier{ upload_worker_list };
            destination_resource_transfer_barrier.addTransitionBarrier(task.destination_descriptor.p_destination_resource, current_destination_resource_state, SplitResourceBarrierFlags::none);
            destination_resource_transfer_barrier.applyBarriers();
        }
    }
}

uint64_t lexgine::core::dx::d3d12::HeapDataUploader::getTransactionSize() const
{
    return m_transaction_size;
}


