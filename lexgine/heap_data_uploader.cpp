#include "heap_data_uploader.h"
#include "device.h"
#include "exception.h"
#include <cassert>

using namespace lexgine::core::dx::d3d12;



HeapDataUploader::HeapDataUploader(Heap& upload_heap, uint64_t offset, uint64_t size) :
    m_heap{ upload_heap },
    m_offset{ offset },
    m_size{ size }
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
        logger(),
        m_heap.device().native()->CreatePlacedResource(upload_heap.native().Get(), static_cast<UINT64>(m_offset), &upload_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&m_upload_buffer)),
        std::bind(&HeapDataUploader::raiseError, this, std::placeholders::_1),
        S_OK
    );


}

void HeapDataUploader::addResourceForUpload(DestinationDescriptor const& destination_descriptor, SourceDescriptor const& source_descriptor)
{
    // Retrieve size of the task
    ID3D12Device* p_device{ nullptr };
    LEXGINE_ERROR_LOG(
        logger(),
        m_upload_buffer->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&p_device)),
        std::bind(&HeapDataUploader::raiseError, this, std::placeholders::_1),
        S_OK
    );
    UINT64 task_size;
    D3D12_RESOURCE_DESC d3d12_destination_resource_descriptor = destination_descriptor.p_destination_resource->native()->GetDesc();
    switch (destination_descriptor.segment_type)
    {
    case HeapDataUploader::DestinationDescriptor::texture:
        p_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, destination_descriptor.segment.subresources.first_subresource, destination_descriptor.segment.subresources.num_subresources,
            0, NULL, NULL, NULL, &task_size);
        break;

    case HeapDataUploader::DestinationDescriptor::buffer:
        p_device->GetCopyableFootprints(&d3d12_destination_resource_descriptor, 0, 1, 0, NULL, NULL, NULL, &task_size);
        break;

    default:
        throw lexgine::core::Exception{ *this, "destination resource is having unknown memory segment" };
    }


    m_upload_tasks.emplace_back(source_descriptor, destination_descriptor, static_cast<uint64_t>(task_size));
}

void HeapDataUploader::upload()
{
    UINT64 task_offset{ 0 };
    for (auto& task : m_upload_tasks)
    {
        ID3D12Device* p_device = nullptr;
        LEXGINE_ERROR_LOG(
            logger(),
            m_upload_buffer->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&p_device)),
            std::bind(&HeapDataUploader::raiseError, this, std::placeholders::_1),
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


        char* p_upload_buffer_mapped_addr{ nullptr };
        LEXGINE_ERROR_LOG(
            logger(),
            m_upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&p_upload_buffer_mapped_addr)),
            std::bind(&HeapDataUploader::raiseError, this, std::placeholders::_1),
            S_OK
        );
        for (uint32_t p = 0; p < num_subresources; ++p)
        {
            UINT64 subresource_offset = p_placed_subresource_footprints[p].Offset;
            for (uint32_t k = 0; k < p_placed_subresource_footprints[p].Footprint.Depth; ++k)
            {

            }
        }


        free(p_placed_subresource_footprints);
        task_offset += resource_to_copy_size;
    }
}


