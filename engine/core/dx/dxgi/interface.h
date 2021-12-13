#ifndef LEXGINE_CORE_DX_DXGI_INTERFACE_H
#define LEXGINE_CORE_DX_DXGI_INTERFACE_H


#include <d3d12.h>
#include <dxgi1_6.h>
#include "preprocessor_tokens.h"


namespace lexgine::core::dx::dxgi {


//! Direct3D 12 feature levels
enum class LEXGINE_CPP_API D3D12FeatureLevel : int
{
    _11_0 = D3D_FEATURE_LEVEL_11_0,
    _11_1 = D3D_FEATURE_LEVEL_11_1,
    _12_0 = D3D_FEATURE_LEVEL_12_0,
    _12_1 = D3D_FEATURE_LEVEL_12_1
};

//! DXGI GPU enumeration preference
enum class LEXGINE_CPP_API DxgiGpuPreference
{
    unspecified = DXGI_GPU_PREFERENCE_UNSPECIFIED,
    minimum_power = DXGI_GPU_PREFERENCE_MINIMUM_POWER,
    high_performance = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
};



}


#endif