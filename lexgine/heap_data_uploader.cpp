#include "heap_data_uploader.h"
#include "device.h"

using namespace lexgine::core::dx::d3d12;



HeapDataUploader::HeapDataUploader(Heap& upload_heap, uint64_t offset, uint64_t size) :
    m_heap{ upload_heap },
    m_offset{ offset },
    m_size{ size }
{
    D3D12_RESOURCE_DESC upload_buffer_desc;
    upload_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_buffer_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
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

void HeapDataUploader::addResourceForUpload(SourceDescriptor const& source_descriptor, DestinationDescriptor const& destination_descriptor)
{
    m_upload_tasks.push_back(upload_task{ source_descriptor, destination_descriptor });
}

void HeapDataUploader::upload()
{
    for (auto& task : m_upload_tasks)
    {
        D3D12_RESOURCE_DESC
    }
}


