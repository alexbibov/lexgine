#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_H

#include "engine/core/class_names.h"
#include "engine/core/entity.h"
#include "lexgine_core_dx_d3d12_fwd.h"

namespace lexgine::core::dx::d3d12 {

class ConstantBuffer : public NamedEntity<class_names::D3D12_ConstantBuffer>
{
public:
    ConstantBuffer(Device const& device, uint64_t size, 
        uint32_t node_mask = 0x1, uint32_t node_exposure_mask = 0x1, 
        bool allow_cross_adapter = false);
    
    ~ConstantBuffer();

    uint64_t mappingAddress(size_t offset = 0U) const;

private:
    std::unique_ptr<CommittedResource> m_resource;
    uint64_t m_buffer_gpu_virtual_address;
};

}

#endif
