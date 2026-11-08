#include <format>

#include "initializer.h"

#include <engine/core/globals.h>
#include <engine/core/misc/log.h>
#include <engine/core/concurrency/thread_affinity.h>
#include <engine/core/rendering/render_thread.h>
#include <engine/core/dx/d3d12_initializer.h>
#include <engine/core/dx/d3d12/d3d12_tools.h>
#include <engine/core/dx/d3d12/device.h>
#include <engine/core/dx/d3d12/dred.h>
#include <engine/core/dx/d3d12/dx_resource_factory.h>
#include <engine/core/dx/d3d12/rendering_tasks.h>
#include <engine/core/dx/d3d12/swap_chain_link.h>
#include <engine/osinteraction/window_handler.h>
#include "engine/conversion/image_loader_pool.h"


namespace lexgine {


namespace
{

core::dx::d3d12::SwapChainDepthBufferFormat convertSwapChainDepthFormatD3d12(core::SwapChainDepthFormat depth_buffer_format)
{
    DXGI_FORMAT dxgiDepthFormat = core::dx::d3d12::d3d12Convert(depth_buffer_format, false);
    return static_cast<core::dx::d3d12::SwapChainDepthBufferFormat>(dxgiDepthFormat);
}

bool areEqual(LUID const& luid1, LUID const& luid2)
{
    return luid1.LowPart == luid2.LowPart && luid1.HighPart == luid2.HighPart;
}

}


EngineSettings::EngineSettings()
    : engine_api{ core::EngineApi::Direct3D12 }
    , debug_mode{ false }
    , enable_profiling{ false }
    , enable_dred{ false }
    , msaa_mode{ core::MSAAMode::none }
    , global_lookup_prefix{ LEXGINE_GLOBAL_LOOKUP_PREFIX }
    , settings_lookup_path{ LEXGINE_SETTINGS_PATH }
    , shaders_lookup_path{ LEXGINE_SHADERS_PATH }
    , logging_output_path{ "" }
    , log_name{ "lexgine.log" }
{

}


Initializer::Initializer(EngineSettings const& settings)
    : m_engine_api{ settings.engine_api }
{
    // the frames are driven by a thread of their own, which claims the rendering role itself
    core::concurrency::registerSimulationThread();

    switch (settings.engine_api)
    {
    case core::EngineApi::Direct3D12:
    {
        core::dx::D3D12EngineSettings d3d12_engine_settings{};
        d3d12_engine_settings.debug_mode = settings.debug_mode;
        d3d12_engine_settings.enable_dred = settings.debug_mode && settings.enable_dred;
        {
            core::dx::d3d12::GpuBasedValidationSettings gpu_based_validation_settings{};
            if (settings.debug_mode)
            {
                gpu_based_validation_settings.disableResourceStateChecks = false;
                gpu_based_validation_settings.enableGpuBasedValidation = false;
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
        d3d12_engine_settings.msaa_mode = settings.msaa_mode;
        d3d12_engine_settings.adapter_enumeration_preference = core::dx::dxgi::DxgiGpuPreference::high_performance;
        d3d12_engine_settings.global_lookup_prefix = settings.global_lookup_prefix;
        d3d12_engine_settings.settings_lookup_path = settings.settings_lookup_path;
        d3d12_engine_settings.global_settings_json_file = settings.global_settings_json_file;
        d3d12_engine_settings.shaders_lookup_path = settings.shaders_lookup_path;
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
        LEXGINE_ASSUME;
    }

    core::GlobalsAttorney<Initializer>::setImageLoaderPool(m_d3d12_initializer->globals(),
        std::make_unique<conversion::ImageLoaderPool>());

    m_device_recreation_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}


Initializer::~Initializer()
{
    m_render_thread.reset();
    if (m_device_recreation_event) CloseHandle(m_device_recreation_event);
}


void Initializer::buildRenderingChain()
{
    core::dx::dxgi::SwapChainDescriptor dxgi_swap_chain_descriptor{
        .format = core::dx::d3d12::d3d12Convert(m_swap_chain_descriptor.color_format),
        .stereo = false,
        .scaling = core::dx::dxgi::SwapChainScaling::none,
        .refreshRate = 60,
        .windowed = m_swap_chain_descriptor.windowed,
        .enable_vsync = m_swap_chain_descriptor.enable_vsync,
        .back_buffer_count = m_swap_chain_descriptor.back_buffer_count
    };

    m_swap_chain = std::make_unique<lexgine::core::dx::dxgi::SwapChain>(
        std::move(m_d3d12_initializer->createSwapChainForCurrentDevice(*m_rendering_window_ptr, dxgi_swap_chain_descriptor))
        );

    m_rendering_tasks = m_d3d12_initializer->createRenderingTasks(*m_rendering_window_ptr,
        [this](uint32_t adapter_id) { return setCurrentDevice(adapter_id); });

    m_swap_chain_link = m_d3d12_initializer->createSwapChainLink(*m_swap_chain,
        convertSwapChainDepthFormatD3d12(m_swap_chain_depth_format), *m_rendering_tasks);

    m_render_thread = core::rendering::RenderThread::create(*m_rendering_window_ptr, *m_swap_chain_link,
        [this]() { SetEvent(m_device_recreation_event); });
    m_render_thread->start();
}

void Initializer::releaseRenderingChain()
{
    m_render_thread.reset();
    m_swap_chain_link.reset();
    m_rendering_tasks.reset();
    m_swap_chain.reset();
}

void Initializer::handleDeviceRecreationRequest()
{
    std::optional<LUID> requested_adapter_luid{};
    {
        std::lock_guard lock{ m_device_recreation_mutex };
        requested_adapter_luid.swap(m_requested_adapter_luid);
    }

    core::dx::d3d12::Device& current_device = m_d3d12_initializer->getCurrentDevice();
    HRESULT const removal_reason = current_device.native()->GetDeviceRemovedReason();
    LUID const current_adapter_luid = current_device.hwAdapter()->getProperties().details.luid;
    LUID const adapter_luid = requested_adapter_luid.value_or(current_adapter_luid);

    if (FAILED(removal_reason))
    {
        core::misc::Log::retrieve()->out(std::format("ERROR: the device was lost (reason 0x{:08X}) and is recreated",
            static_cast<uint32_t>(removal_reason)), core::misc::LogMessageType::error);

        std::string const removal_description = core::dx::d3d12::describeDeviceRemoval(current_device);
        if (!removal_description.empty())
        {
            core::misc::Log::retrieve()->out("Device removed extended data:\n" + removal_description,
                core::misc::LogMessageType::error);
        }
    }
    else if (areEqual(adapter_luid, current_adapter_luid))
    {
        core::misc::Log::retrieve()->out("The requested adapter is already in use", core::misc::LogMessageType::information);
        return;
    }
    else
    {
        core::misc::Log::retrieve()->out("The device is recreated on the requested adapter",
            core::misc::LogMessageType::information);
    }

    m_render_thread.reset();
    std::optional<core::dx::d3d12::RenderingTasks::SceneSource> const scene_source = m_rendering_tasks->currentSceneSource();
    releaseRenderingChain();

    if (!m_d3d12_initializer->recreateDevice(adapter_luid))
    {
        core::misc::Log::retrieve()->out("ERROR: unable to recreate the lost device", core::misc::LogMessageType::error);
        return;
    }

    buildRenderingChain();

    if (scene_source && !m_rendering_tasks->requestSceneLoad(*scene_source))
    {
        core::misc::Log::retrieve()->out("ERROR: unable to schedule reloading of the scene " + scene_source->path,
            core::misc::LogMessageType::error);
    }
}

bool Initializer::setCurrentDevice(uint32_t adapter_id)
{
    switch (m_engine_api)
    {
    case core::EngineApi::Direct3D12:
    {
        if (!m_rendering_window_ptr) return m_d3d12_initializer->setCurrentDevice(adapter_id);

        auto const& adapter_enumerator = m_d3d12_initializer->globals().dxResourceFactory().hardwareAdapterEnumerator();
        if (adapter_id >= adapter_enumerator.getAdapterCount()) return false;

        {
            std::lock_guard lock{ m_device_recreation_mutex };
            m_requested_adapter_luid = adapter_enumerator[adapter_id]->getProperties().details.luid;
        }

        SetEvent(m_device_recreation_event);
        return true;
    }

    default:
        return false;
    }
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
        m_rendering_window_ptr = &window;
        m_swap_chain_descriptor = swap_chain_desc;
        m_swap_chain_depth_format = depth_format;

        buildRenderingChain();

        m_window_handler = std::make_unique<osinteraction::WindowHandler>(
            std::move(osinteraction::WindowHandlerAttorney<Initializer>::makeWindowHandler(m_d3d12_initializer->globals(), window)
            ));
        osinteraction::WindowHandlerAttorney<Initializer>::addWaitableHandler(*m_window_handler, m_device_recreation_event,
            [this]() { handleDeviceRecreationRequest(); });

        return m_window_handler.get();
    }


    case lexgine::core::EngineApi::Vulkan:
    case lexgine::core::EngineApi::Metal:
    case lexgine::core::EngineApi::OpenGL46:
        return nullptr;
    }

    return nullptr;
}


core::Globals& Initializer::globals()
{
    switch (m_engine_api)
    {
        case core::EngineApi::Direct3D12:
            return m_d3d12_initializer->globals();

        default:
            LEXGINE_ASSUME;
    }

    return m_d3d12_initializer->globals();
}

}