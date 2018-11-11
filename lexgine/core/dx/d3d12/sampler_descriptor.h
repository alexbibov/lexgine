#ifndef LEXGINE_CORE_DX_D3D12_SAMPLER_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_SAMPLER_DESCRIPTOR_H

#include <d3d12.h>
#include <cstdint>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/math/vector_types.h"


namespace lexgine::core::dx::d3d12{

class SamplerDescriptor final
{
public:
    SamplerDescriptor(FilterPack const& filter, math::Vector4f const& border_color);

    D3D12_SAMPLER_DESC nativeDescriptor() const;

private:
    D3D12_SAMPLER_DESC m_native;
};

}

#endif
