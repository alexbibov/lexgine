#ifndef LEXGINE_CORE_DX_D3D12_SAMPLER_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_SAMPLER_DESCRIPTOR_H

#include <cstdint>

#include <d3d12.h>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/math/vector_types.h"
#include "hashable_descriptor.h"


namespace lexgine::core::dx::d3d12{

class SamplerDescriptor final : public HashableDescriptor<D3D12_SAMPLER_DESC>
{
public:
    SamplerDescriptor(FilterPack const& filter, math::Vector4f const& border_color);

    D3D12_SAMPLER_DESC nativeDescriptor() const;

private:
    D3D12_SAMPLER_DESC m_native;
};

}

#endif
