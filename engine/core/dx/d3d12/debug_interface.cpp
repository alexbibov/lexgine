#include "debug_interface.h"
#include "engine/core/exception.h"

using namespace lexgine::core::dx::d3d12;

DebugInterface* DebugInterface::m_p_myself = nullptr;

DebugInterface const* DebugInterface::create(GpuBasedValidationSettings const& gpu_based_validation_settings)
{
    if (!m_p_myself)
    {
        m_p_myself = new DebugInterface(gpu_based_validation_settings);
    }

    return m_p_myself;
}

DebugInterface const* DebugInterface::retrieve() { return m_p_myself; }

void DebugInterface::shutdown()
{
    if (m_p_myself)
    {
        delete m_p_myself;
        m_p_myself = nullptr;
    }
}


DebugInterface::DebugInterface(GpuBasedValidationSettings const& gpu_based_validation_settings)
    :m_gpu_validation_settings{ gpu_based_validation_settings }
{
    ID3D12Debug* p_d3d12_debug{ nullptr };

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        D3D12GetDebugInterface(IID_PPV_ARGS(&p_d3d12_debug)),
        S_OK);


    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        p_d3d12_debug->QueryInterface(IID_PPV_ARGS(&m_d3d12_debug)),
        S_OK);

    p_d3d12_debug->Release();

    m_d3d12_debug->EnableDebugLayer();
    m_d3d12_debug->SetEnableGPUBasedValidation(gpu_based_validation_settings.enableGpuBasedValidation);
    
    if (gpu_based_validation_settings.enableGpuBasedValidation)
    {
        ID3D12Debug2* p_d3d12_debug_2{ nullptr };
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_d3d12_debug->QueryInterface(IID_PPV_ARGS(&p_d3d12_debug_2)),
            S_OK
        );

        if (gpu_based_validation_settings.disableResourceStateChecks)
        {
            p_d3d12_debug_2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING);
        }

        p_d3d12_debug_2->Release();
    }
}