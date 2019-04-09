#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_DATA_STREAM_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_DATA_STREAM_H

#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "upload_buffer_allocator.h"

namespace lexgine::core::dx::d3d12 {

class ConstantDataStream
{
public:
    ConstantDataStream(Globals& globals);

    uint64_t totalCapacity() const;

    uint64_t allocate(uint64_t size);

    void update(uint64_t dst_gpu_address, ConstantDataMapper const& data_mapper,
        ConstantBufferReflection const& reflection);

    void allocateAndUpdate(ConstantDataMapper const& data_mapper, ConstantBufferReflection const& reflection);

private:
    PerFrameUploadDataStreamAllocator m_allocator;
};

}

#endif
