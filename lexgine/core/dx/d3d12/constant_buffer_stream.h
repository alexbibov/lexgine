#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_STREAM_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_STREAM_H

#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "upload_buffer_allocator.h"

namespace lexgine::core::dx::d3d12 {

class ConstantBufferStream
{
public:
    ConstantBufferStream(Globals& globals);

    uint64_t totalCapacity() const;

    PerFrameUploadDataStreamAllocator::address_type allocate(uint64_t size);

    void update(PerFrameUploadDataStreamAllocator::address_type const& destination_data_block, ConstantBufferDataMapper const& data_mapper);

    PerFrameUploadDataStreamAllocator::address_type allocateAndUpdate(ConstantBufferDataMapper const& data_mapper);

private:
    PerFrameUploadDataStreamAllocator m_allocator;
};

}

#endif
