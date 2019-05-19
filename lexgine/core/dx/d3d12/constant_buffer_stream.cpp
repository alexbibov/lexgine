#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "dx_resource_factory.h"
#include "device.h"
#include "constant_buffer_stream.h"
#include "constant_buffer_data_mapper.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

std::string const ConstantBufferStream::c_constant_section_name = "constant_upload_section";

ConstantBufferStream::ConstantBufferStream(Globals& globals)
    : m_allocator{ nullptr }
{
    GlobalSettings const& global_settings = *globals.get<GlobalSettings>();
    float streamed_constant_data_partitioning = global_settings.getStreamedConstantDataPartitioning();
    size_t upload_buffer_total_size = global_settings.getUploadHeapCapacity();
    size_t constant_data_section_size = static_cast<size_t>(upload_buffer_total_size * streamed_constant_data_partitioning);

    DxResourceFactory& dx_resources = *globals.get<DxResourceFactory>();
    Device& device = *globals.get<Device>();

    auto section_in_upload_heap =
        dx_resources.allocateSectionInUploadHeap(dx_resources.retrieveUploadHeap(device),
            c_constant_section_name, constant_data_section_size);

    if (!section_in_upload_heap.isValid())
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Unable to reserve section \"" + c_constant_section_name + "\" in the upload heap");
    }

    UploadHeapPartition const& partition = static_cast<UploadHeapPartition const&>(section_in_upload_heap);

    m_allocator = std::make_unique<PerFrameUploadDataStreamAllocator>(globals, partition.offset, partition.size, 
        dx_resources.retrieveFrameProgressTracker(device));
}

uint64_t ConstantBufferStream::totalCapacity() const
{
    return m_allocator->totalCapacity();
}

PerFrameUploadDataStreamAllocator::address_type ConstantBufferStream::allocate(uint64_t size)
{
    return m_allocator->allocate(size);
}

void ConstantBufferStream::update(PerFrameUploadDataStreamAllocator::address_type const& destination_data_block, ConstantBufferDataMapper const& data_mapper)
{
    data_mapper.writeAllBoundData(reinterpret_cast<size_t>(destination_data_block->cpuAddress()));
}

PerFrameUploadDataStreamAllocator::address_type ConstantBufferStream::allocateAndUpdate(ConstantBufferDataMapper const& data_mapper)
{
    auto allocation = m_allocator->allocate(data_mapper.mappedDataSize());
    update(allocation, data_mapper);
    return allocation;
}
