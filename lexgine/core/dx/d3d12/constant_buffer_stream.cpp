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

uint64_t ConstantBufferStream::allocate(uint64_t size)
{
    return reinterpret_cast<uint64_t>(m_allocator.allocate(size)->address());
}

void ConstantBufferStream::update(uint64_t dst_gpu_address, ConstantBufferDataMapper const& data_mapper)
{
    data_mapper.update(dst_gpu_address);
}

void ConstantBufferStream::allocateAndUpdate(ConstantBufferDataMapper const& data_mapper)
{
    auto allocation = m_allocator.allocate(data_mapper.requiredDestinationBufferCapacity());
    update(reinterpret_cast<uint64_t>(allocation->address()), data_mapper);
}
