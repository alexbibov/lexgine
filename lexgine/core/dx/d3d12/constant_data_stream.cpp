#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "dx_resource_factory.h"
#include "device.h"
#include "constant_data_stream.h"


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

ConstantDataStream::ConstantDataStream(Globals& globals)
    : m_allocator{ globals, 0U, fetchConstantStreamSize(globals),
        globals.get<DxResourceFactory>()->retrieveFrameProgressTracker(*globals.get<Device>()) }
{
}
