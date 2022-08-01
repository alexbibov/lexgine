#include "initializer.h"
#include "engine/core/dx/d3d12_initializer.h"
#include "engine/core/dx/d3d12/d3d12_tools.h"

namespace lexgine {

EngineSettings::EngineSettings()
    : engine_api{ core::EngineApi::Direct3D12 }
    , debug_mode{ false }
    , enable_profiling{ false }
    , global_lookup_prefix{ "" }
    , settings_lookup_path{ "" }
    , logging_output_path{ "" }
    , log_name{ "lexgine.log" }
{

}


Initializer::Initializer(EngineSettings const& settings)
    : m_engine_api{ settings.engine_api }
{
    switch (settings.engine_api)
    {
    case core::EngineApi::Direct3D12:
    {
        core::dx::D3D12EngineSettings d3d12_engine_settings{};
        d3d12_engine_settings.debug_mode = settings.debug_mode;
        {
            core::dx::d3d12::GpuBasedValidationSettings gpu_based_validation_settings{};
            if (settings.debug_mode)
            {
                gpu_based_validation_settings.disableResourceStateChecks = false;
                gpu_based_validation_settings.enableGpuBasedValidation = true;
                gpu_based_validation_settings.enableSynchronizedCommandQueueValidation = true;
            }
            else
            {
                gpu_based_validation_settings.disableResourceStateChecks = true;
                gpu_based_validation_settings.enableGpuBasedValidation = false;
                gpu_based_validation_settings.enableSynchronizedCommandQueueValidation = false;
            }
            d3d12_engine_settings.gpu_based_validation_settings = gpu_based_validation_settings;
        }
        d3d12_engine_settings.enable_profiling = settings.enable_profiling;
        d3d12_engine_settings.adapter_enumeration_preference = core::dx::dxgi::DxgiGpuPreference::high_performance;
        d3d12_engine_settings.global_lookup_prefix = settings.global_lookup_prefix;
        d3d12_engine_settings.settings_lookup_path = settings.settings_lookup_path;
        d3d12_engine_settings.global_settings_json_file = settings.global_settings_json_file;
        d3d12_engine_settings.logging_output_path = settings.logging_output_path;
        d3d12_engine_settings.log_name = settings.log_name;
        m_d3d12_initializer = std::make_unique<core::dx::D3D12Initializer>(d3d12_engine_settings);
        break;
    }

    case core::EngineApi::Vulkan:
    {
        break;
    }

    default:
        break;
    }
}


bool Initializer::setCurrentDevice(uint32_t adapter_id)
{
    switch (m_engine_api)
    {
    case core::EngineApi::Direct3D12:
        m_d3d12_initializer->setCurrentDevice(adapter_id);
        break;

    case core::EngineApi::Vulkan:
        return false;
        break;
    }

    return false;
}


uint32_t Initializer::getAdapterCount() const
{
    switch (m_engine_api)
    {
    case core::EngineApi::Direct3D12:
        return m_d3d12_initializer->getAdapterCount();
    case core::EngineApi::Vulkan:
        return -1;
    }
    return -1;
}


std::unique_ptr<core::SwapChain> Initializer::createSwapChainForCurrentDevice(osinteraction::WindowHandler& window_handler, core::SwapChainDescriptor const& descriptor) const
{
    switch (m_engine_api)
    {
    case core::EngineApi::Direct3D12:
    {
        core::dx::dxgi::SwapChainDescriptor dxgi_swap_chain_descriptor{
            .format = core::dx::d3d12::d3d12Convert(descriptor.color_format),
            .stereo = false,
            .scaling = core::dx::dxgi::SwapChainScaling::none,
            .refreshRate = 60,
            .windowed = descriptor.windowed,
            .enable_vsync = descriptor.enable_vsync,
            .back_buffer_count = descriptor.back_buffer_count
        };
        return std::unique_ptr<core::SwapChain>{new core::dx::dxgi::SwapChain{ std::move(m_d3d12_initializer->createSwapChainForCurrentDevice(*window_handler.attachedWindow(), dxgi_swap_chain_descriptor)) }};
    }
    case core::EngineApi::Vulkan:
        return nullptr;
    }

    return nullptr;
}


}