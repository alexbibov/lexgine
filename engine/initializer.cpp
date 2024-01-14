#include "initializer.h"
#include <engine/core/dx/d3d12_initializer.h>
#include <engine/core/dx/d3d12/d3d12_tools.h>
#include <engine/core/dx/d3d12/device.h>
#include <engine/core/dx/d3d12/rendering_tasks.h>
#include <engine/core/dx/d3d12/swap_chain_link.h>
#include <engine/osinteraction/window_handler.h>


namespace lexgine {


namespace
{

core::dx::d3d12::SwapChainDepthBufferFormat convertSwapChainDepthFormatD3d12(core::SwapChainDepthFormat depth_buffer_format)
{
    DXGI_FORMAT dxgiDepthFormat = core::dx::d3d12::d3d12Convert(depth_buffer_format, false);
    return static_cast<core::dx::d3d12::SwapChainDepthBufferFormat>(dxgiDepthFormat);
}

}


EngineSettings::EngineSettings()
    : engine_api{ core::EngineApi::Direct3D12 }
    , debug_mode{ false }
    , enable_profiling{ false }
    , global_lookup_prefix{ LEXGINE_GLOBAL_LOOKUP_PREFIX }
    , settings_lookup_path{ LEXGINE_SETTINGS_PATH }
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


Initializer::~Initializer() = default;


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


DeviceDetails Initializer::getDeviceDetails() const
{
    auto adapter = m_d3d12_initializer->getCurrentDevice().hwAdapter();
    auto adapter_properties = adapter->getProperties();

    DeviceDetails rv{};

    rv.description = adapter_properties.details.name;
    rv.dedicated_video_memory = adapter_properties.details.dedicated_video_memory;
    rv.dedicated_system_memory = adapter_properties.details.dedicated_system_memory;
    rv.shared_system_memory = adapter_properties.details.shared_system_memory;
    rv.d3d12_feature_level = adapter_properties.d3d12_feature_level;

    return rv;
}


osinteraction::WindowHandler* Initializer::createWindowHandler(osinteraction::windows::Window& window, core::SwapChainDescriptor const& swap_chain_desc, core::SwapChainDepthFormat depth_format)
{
    switch (m_engine_api)
    {
    case lexgine::core::EngineApi::Direct3D12:
    {
        core::dx::dxgi::SwapChainDescriptor dxgi_swap_chain_descriptor{
            .format = core::dx::d3d12::d3d12Convert(swap_chain_desc.color_format),
            .stereo = false,
            .scaling = core::dx::dxgi::SwapChainScaling::none,
            .refreshRate = 60,
            .windowed = swap_chain_desc.windowed,
            .enable_vsync = swap_chain_desc.enable_vsync,
            .back_buffer_count = swap_chain_desc.back_buffer_count
        };

        m_swap_chain = std::make_unique<lexgine::core::dx::dxgi::SwapChain>(
            std::move(m_d3d12_initializer->createSwapChainForCurrentDevice(window, dxgi_swap_chain_descriptor))
            );

        m_rendering_tasks = m_d3d12_initializer->createRenderingTasks();

        m_swap_chain_link = m_d3d12_initializer->createSwapChainLink(*m_swap_chain, convertSwapChainDepthFormatD3d12(depth_format), *m_rendering_tasks);

        m_window_handler = std::make_unique<osinteraction::WindowHandler>(
            std::move(osinteraction::WindowHandlerAttorney<Initializer>::makeWindowHandler(m_d3d12_initializer->globals(), window, *m_swap_chain_link)
            ));

        return m_window_handler.get();
    }


    case lexgine::core::EngineApi::Vulkan:
    case lexgine::core::EngineApi::Metal:
    case lexgine::core::EngineApi::OpenGL46:
        return nullptr;
    }

    return nullptr;
}


}