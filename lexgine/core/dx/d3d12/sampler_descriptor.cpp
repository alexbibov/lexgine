#include "sampler_descriptor.h"

#include "lexgine/core/filter.h"
#include "d3d12_tools.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

SamplerDescriptor::SamplerDescriptor(FilterPack const& filter, math::Vector4f const& border_color)
{
    auto uv_address = filter.getWrapModeUV();
    m_native.AddressU = d3d12Convert(uv_address.first);
    m_native.AddressV = d3d12Convert(uv_address.second);
    m_native.AddressW = d3d12Convert(filter.getWrapModeW());

    memcpy(m_native.BorderColor, border_color.getDataAsArray(), sizeof(float) * 4);

    m_native.ComparisonFunc = d3d12Convert(filter.getComparisonFunction());
    m_native.Filter = d3d12Convert(filter.MinFilter(), filter.MagFilter(), filter.isComparison());
    m_native.MaxAnisotropy = filter.getMaximalAnisotropyLevel();
    
    auto min_max_lod = filter.getMinMaxLOD();
    m_native.MinLOD = min_max_lod.first;
    m_native.MaxLOD = min_max_lod.second;

    m_native.MipLODBias = filter.getMipLODBias();
}

D3D12_SAMPLER_DESC SamplerDescriptor::nativeDescriptor() const
{
    return m_native;
}
