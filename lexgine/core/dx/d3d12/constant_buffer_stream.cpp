#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "dx_resource_factory.h"
#include "device.h"
#include "constant_buffer_stream.h"
#include "constant_buffer_data_mapper.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

namespace {

size_t fetchConstantStreamSize(Globals const& globals)
{
    GlobalSettings const& global_settings = *globals.get<GlobalSettings>();
    float streamed_constant_data_partitioning = global_settings.getStreamedConstantDataPartitioning();
    size_t upload_buffer_total_size = global_settings.getUploadHeapCapacity();
    return static_cast<size_t>(upload_buffer_total_size*streamed_constant_data_partitioning);
}

}

ConstantBufferStream::ConstantBufferStream(Globals& globals)
    : m_allocator{ globals, 0U, fetchConstantStreamSize(globals),
        globals.get<DxResourceFactory>()->retrieveFrameProgressTracker(*globals.get<Device>()) }
{
}

uint64_t ConstantBufferStream::totalCapacity() const
{
    return m_allocator.totalCapacity();
}

PerFrameUploadDataStreamAllocator::address_type ConstantBufferStream::allocate(uint64_t size)
{
    return m_allocator.allocate(size);
}

void ConstantBufferStream::update(PerFrameUploadDataStreamAllocator::address_type const& destination_data_block, ConstantBufferDataMapper const& data_mapper)
{
    data_mapper.writeAllBoundData(reinterpret_cast<size_t>(destination_data_block->cpuAddress()));
}

PerFrameUploadDataStreamAllocator::address_type ConstantBufferStream::allocateAndUpdate(ConstantBufferDataMapper const& data_mapper)
{
    auto allocation = m_allocator.allocate(data_mapper.mappedDataSize());
    update(allocation, data_mapper);
    return allocation;
}
