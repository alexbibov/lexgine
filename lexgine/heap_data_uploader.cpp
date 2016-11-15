#include "heap_data_uploader.h"
#include "device.h"
#include "exception.h"
#include <cassert>

using namespace lexgine::core::dx::d3d12;



HeapDataUploader::HeapDataUploader(Heap& upload_heap, uint64_t offset, uint64_t size) :
    m_heap{ upload_heap },
    m_offset{ offset },
    m_size{ size },
    m_transaction_size{ 0U }
{
    D3D12_RESOURCE_DESC upload_buffer_desc;
    upload_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_buffer_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    upload_buffer_desc.Width = static_cast<UINT64>(size);
    upload_buffer_desc.Height = 1;
    upload_buffer_desc.DepthOrArraySize = 1;
    upload_buffer_desc.MipLevels = 1;
    upload_buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    upload_buffer_desc.SampleDesc.Count = 1;
    upload_buffer_desc.SampleDesc.Quality = 0;
    upload_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    upload_buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    LEXGINE_ERROR_LOG(
        this,
        m_heap.device().native()->CreatePlacedResource(upload_heap.native().Get(), static_cast<UINT64>(m_offset), &upload_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_upload_buffer)),
        S_OK
    );


}

void HeapDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor, SourceDescriptor const& source_descriptor)
{
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->native()->GetDesc();
    D3D12_RESOURCE_DESC d3d12_upload_resource_descriptor = m_upload_buffer->GetDesc();
    UINT64 task_size;

    // Retrieve size of the task
    ID3D12Device* p_device{ nullptr };
    LEXGINE_ERROR_LOG(
        this,
        m_upload_buffer->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&p_device)),
        S_OK
    );


    switch (destination_descriptor.segment_type)
    {
    case HeapDataUploader::DestinationDescriptor::texture:
    {
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
            m_upload_buffer->Map(0, NULL, reinterpret_cast<void**>(&p_upload_buffer_addr)),
            S_OK
        );


        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            char* p_subresource_offset = p_upload_buffer_addr + p_placed_subresource_footprints[p].Offset;
            size_t const resource_slice_size = p_subresource_num_rows[p] * p_placed_subresource_footprints[p].Footprint.RowPitch;

            for (uint32_t k = 0; k < p_placed_subresource_footprints[p].Footprint.Depth; ++k)
            {
                char* p_subresource_slice_offset = p_subresource_offset + resource_slice_size*k;
                for (uint32_t i = 0; i < p_subresource_num_rows[p]; ++i)
                {
                    char* p_subresource_row_offset = p_subresource_slice_offset + p_placed_subresource_footprints[p].Footprint.RowPitch*i;
                    memcpy(p_subresource_row_offset, static_cast<char*>(source_descriptor.p_source_data) + source_descriptor.slice_pitch*k + source_descriptor.row_pitch*i, p_subresource_row_size_in_bytes[p]);
                }
            }
        }


        m_upload_buffer->Unmap(0, NULL);

        m_upload_tasks.emplace_back(source_descriptor, destination_descriptor, static_cast<uint64_t>(task_size), placed_subresource_footprints_buffer);
        break;
    }

    case HeapDataUploader::DestinationDescriptor::buffer:
    {
        size_t const subresource_copy_footprints_size = sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64);
        DataChunk placed_subresource_footprints_buffer{ subresource_copy_footprints_size };
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprint = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(placed_subresource_footprints_buffer.data());
        UINT* p_subresource_num_rows = reinterpret_cast<UINT*>(p_placed_subresource_footprint + 1);
        UINT64* p_subresource_row_size_in_bytes = reinterpret_cast<UINT64*>(p_subresource_num_rows + 1);

        p_device->GetCopyableFootprints(&d3d12_upload_resource_descriptor, 0, 1, m_transaction_size, p_placed_subresource_footprint, p_subresource_num_rows, p_subresource_row_size_in_bytes, &task_size);

        // Copy the contents of the current upload task into upload buffer
        char* p_upload_buffer_addr{ nullptr };
        LEXGINE_ERROR_LOG(
            this,
            m_upload_buffer->Map(0, NULL, reinterpret_cast<void**>(&p_upload_buffer_addr)),
            S_OK
        );
        memcpy(p_upload_buffer_addr + p_placed_subresource_footprint->Offset, source_descriptor.p_source_data, p_placed_subresource_footprint->Footprint.Width);
        m_upload_buffer->Unmap(0, NULL);

        m_upload_tasks.emplace_back(source_descriptor, destination_descriptor, static_cast<uint64_t>(task_size), placed_subresource_footprints_buffer);
        break;
    }

    default:
        throw lexgine::core::Exception{ *this, "destination resource is having unknown memory segment" };
    }


    m_transaction_size += task_size;
}

void HeapDataUploader::upload()
{
    /*UINT64 task_offset{ 0 };
    for (auto& task : m_upload_tasks)
    {
        ID3D12Device* p_device = nullptr;
        LEXGINE_ERROR_LOG(
            this,
            m_upload_buffer->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&p_device)),
            S_OK
        );

        D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = task.destination_descriptor.p_destination_resource->native()->GetDesc();
        D3D12_RESOURCE_DESC d3d12_upload_resource_descriptor = m_upload_buffer->GetDesc();

        uint32_t num_subresources = task.destination_descriptor.segment_type == DestinationDescriptor::DestinationDescriptorSegmentType::buffer ? 1U : task.destination_descriptor.segment.subresources.num_subresources;
        size_t copyable_footprints_buffer_size = (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * num_subresources;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_placed_subresource_footprints = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(malloc(copyable_footprints_buffer_size));
        UINT* p_subresources_num_rows = reinterpret_cast<UINT*>(p_placed_subresource_footprints + num_subresources);
        UINT64* p_subresources_row_size_in_bytes = reinterpret_cast<UINT64*>(p_subresources_num_rows + num_subresources);
        UINT64 resource_to_copy_size = task.task_size;

        switch (task.destination_descriptor.segment_type)
        {
        case HeapDataUploader::DestinationDescriptor::texture:
            p_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, task.destination_descriptor.segment.subresources.first_subresource,
                task.destination_descriptor.segment.subresources.num_subresources, task_offset, p_placed_subresource_footprints, p_subresources_num_rows, p_subresources_row_size_in_bytes, NULL);
            break;

        case HeapDataUploader::DestinationDescriptor::buffer:
            p_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, 0, 1, task_offset, p_placed_subresource_footprints, p_subresources_num_rows, p_subresources_row_size_in_bytes, NULL);
            break;

        default:
            throw lexgine::core::Exception{ *this, "destination resource is having unknown memory segment" };
        }


        assert(d3d12_upload_resource_descriptor.Width >= resource_to_copy_size + task_offset);


        // Copy the source data to upload buffer
        char* p_upload_buffer_mapped_addr{ nullptr };
        LEXGINE_ERROR_LOG(
            this,
            m_upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&p_upload_buffer_mapped_addr)),
            S_OK
        );
        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            UINT64 subresource_offset = p_placed_subresource_footprints[p].Offset;
            uint64_t const slice_size = p_placed_subresource_footprints[p].Footprint.RowPitch * p_subresources_num_rows[p];

            for (uint32_t k = 0; k < p_placed_subresource_footprints[p].Footprint.Depth; ++k)
            {
                char* p_slice_offset = p_upload_buffer_mapped_addr + slice_size*k;
                for (uint32_t j = 0; j < p_subresources_num_rows[p]; ++j)
                {
                    char* p_row_offset = p_slice_offset + p_placed_subresource_footprints[p].Footprint.RowPitch*j;
                    memcpy(p_row_offset, static_cast<char*>(task.source_descriptor.p_source_data) + task.source_descriptor.slice_pitch*k + task.source_descriptor.row_pitch*j, p_subresources_row_size_in_bytes[p]);
                }
            }
        }
        m_upload_buffer->Unmap(0, nullptr);





        free(p_placed_subresource_footprints);
        task_offset += resource_to_copy_size;
    }*/
}


