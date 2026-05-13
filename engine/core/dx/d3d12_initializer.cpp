#include <windows.h>
#include <cwchar>
#include <system_error>

#include "d3d12_initializer.h"

#include "engine/core/globals.h"
#include "engine/core/global_settings.h"

#include "engine/build_info.h"
#include "engine/core/misc/misc.h"
#include "engine/core/misc/log.h"

#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/device.h"

#include "engine/core/gpu_data_blob_on_disk_streamed_cache.h"
#include "engine/core/dx/d3d12/task_caches/hlsl_compilation_task_cache.h"
#include "engine/core/dx/d3d12/task_caches/pso_compilation_task_cache.h"
#include "engine/core/dx/d3d12/task_caches/root_signature_blob_cache.h"

#include "engine/conversion/texture_converter.h"


using namespace lexgine;
using namespace lexgine::core;
namespace lexgine::core::dx {


D3D12EngineSettings::D3D12EngineSettings()
    : debug_mode{ false }
    , enable_profiling{ false }
    , msaa_mode{ MSAAMode::none }
    , adapter_enumeration_preference{ dx::dxgi::DxgiGpuPreference::high_performance }
    , global_settings_json_file{ "global_settings.json" }
    , log_name{ "lexgine.log" }
    , log_level{ misc::LogMessageType::trace }
{

}


D3D12Initializer::D3D12Initializer(D3D12EngineSettings const& settings)
    : m_engine_api{ EngineApi::Direct3D12 }
{
    // std::string corrected_logging_output_path = correct_path(settings.logging_output_path);
    // std::string corrected_global_lookup_prefix = correct_path(settings.global_lookup_prefix);
    // std::string corrected_settings_lookup_path = correct_path(settings.settings_lookup_path);
    // std::string corrected_shaders_lookup_path = correct_path(settings.shaders_lookup_path);

    {
        wchar_t host_computer_name[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD host_computer_name_length = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerName(host_computer_name, &host_computer_name_length))
        {
            wcscpy_s(host_computer_name, L"UNKNOWN_HOST");
        }
        std::string log_name{ std::string{ PROJECT_CODE_NAME } + " v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
        + std::to_string(PROJECT_VERSION_MINOR) + " rev." + std::to_string(PROJECT_VERSION_BUILD)
        + "(" + std::to_string(PROJECT_VERSION_REVISION) + ")" + " (" + misc::wstringToAsciiString(host_computer_name) + ")" };

        misc::Log::create(settings.logging_output_path / (settings.log_name.stem().string() + ".html"), settings.log_name.string(), settings.log_level);
    }

    // Load the global settings
    m_global_settings.reset(new GlobalSettings{ settings.global_lookup_prefix / settings.settings_lookup_path / settings.global_settings_json_file });

    // Correct shader lookup directories
	{
		std::vector<std::filesystem::path> new_shader_lookup_directories;
		for (auto& shader_lookup_path : m_global_settings->getShaderLookupDirectories())
		{
			new_shader_lookup_directories.push_back(settings.global_lookup_prefix / shader_lookup_path);
		}
		m_global_settings->clearShaderLookupDirectories();
		for (auto& new_shader_lookup_path : new_shader_lookup_directories)
		{
			m_global_settings->addShaderLookupDirectory(new_shader_lookup_path);
		}
		m_global_settings->addShaderLookupDirectory(settings.global_lookup_prefix);
	}

    // Correct cache path
    {
        std::filesystem::path corrected_cache_path = settings.global_lookup_prefix / m_global_settings->getCacheDirectory();
        m_global_settings->setCacheDirectory(corrected_cache_path);

        std::error_code ec;
        std::filesystem::create_directories(corrected_cache_path, ec);
        if (ec)
        {
            misc::Log::retrieve()->out("Unable to create cache path \"" + corrected_cache_path.string() + "\". Caching may not function properly, which "
                "may deteriorate performance of the system", misc::LogMessageType::exclamation);
        }
    }

    // Set profiling enable state
    m_global_settings->setIsProfilingEnabled(settings.enable_profiling);

    // Initialize resource factory
    m_resource_factory.reset(new dx::d3d12::DxResourceFactory{ *m_global_settings, settings.debug_mode, settings.gpu_based_validation_settings,
        settings.adapter_enumeration_preference });

    // Initialize caches
    {
        m_data_cache = std::make_unique<GpuDataBlobOnDiskStreamedCache>(*m_global_settings, false);
        m_shader_cache = std::make_unique<dx::d3d12::task_caches::HLSLCompilationTaskCache>();
        m_pso_cache = std::make_unique<dx::d3d12::task_caches::PSOCompilationTaskCache>();
        m_rs_cache = std::make_unique<dx::d3d12::task_caches::RootSignatureBlobCache>();
    }
    buildGlobals();
    setCurrentDevice(0);
}

D3D12Initializer::~D3D12Initializer()
{
    m_texture_converter.reset();
    m_shader_cache.reset();
    m_rs_cache.reset();
    m_pso_cache.reset();
    m_data_cache.reset();
    m_resource_factory.reset();
    m_globals.reset();
    m_global_settings.reset();

    misc::Log::retrieve()->out("Alive engine objects: " + std::to_string(Entity::aliveEntities()), misc::LogMessageType::information);

    // Logger must be shutdown the last since many objects may still log stuff on destruction
    misc::Log::shutdown();
}


core::Globals& D3D12Initializer::globals()
{
    return *m_globals;
}

bool D3D12Initializer::setCurrentDevice(uint32_t adapter_id)
{
    auto& hw_adapter_enumerator = m_resource_factory->hardwareAdapterEnumerator();
    if (adapter_id >= hw_adapter_enumerator.getAdapterCount()) return false;

    dx::d3d12::Device& device_ref = hw_adapter_enumerator[adapter_id]->device();
    m_globals->put(&device_ref);

    // Texture converter is re-created each time the device is switched as it relies on batch texture uploader that is device dependent
    {
        m_texture_converter.reset(new conversion::TextureConverter{ *m_globals });
        m_globals->put(m_texture_converter.get());
    }
    return true;
}

dx::d3d12::Device& D3D12Initializer::getCurrentDevice() const
{
    return *m_globals->get<dx::d3d12::Device>();
}

void D3D12Initializer::setWARPAdapterAsCurrent() const
{
    dx::d3d12::Device& warp_device_ref = m_resource_factory->hardwareAdapterEnumerator().getWARPAdapter()->device();
    m_globals->put(&warp_device_ref);
}

uint32_t D3D12Initializer::getAdapterCount() const
{
    return m_resource_factory->hardwareAdapterEnumerator().getAdapterCount();
}

dx::dxgi::SwapChain D3D12Initializer::createSwapChainForCurrentDevice(osinteraction::windows::Window& window, dx::dxgi::SwapChainDescriptor const& desc) const
{
    return getCurrentDevice().hwAdapter()->createSwapChain(window, desc);
}

std::unique_ptr<dx::d3d12::RenderingTasks> D3D12Initializer::createRenderingTasks() const
{
    return std::make_unique<dx::d3d12::RenderingTasks>(*m_globals);
}

std::shared_ptr<dx::d3d12::SwapChainLink> D3D12Initializer::createSwapChainLink(dx::dxgi::SwapChain& target_swap_chain,
    dx::d3d12::SwapChainDepthBufferFormat depth_buffer_format,
    dx::d3d12::RenderingTasks& source_rendering_tasks) const
{
    auto rv = dx::d3d12::SwapChainLink::create(*m_globals, target_swap_chain, depth_buffer_format);
    rv->linkRenderingTasks(&source_rendering_tasks);
    return rv;
}

void D3D12Initializer::buildGlobals()
{
    m_globals = std::make_unique<Globals>();
    m_globals->put(const_cast<EngineApi*>(&m_engine_api));
    m_globals->put(m_global_settings.get());
    m_globals->put(m_resource_factory.get());
    m_globals->put(m_data_cache.get());
    m_globals->put(m_shader_cache.get());
    m_globals->put(m_pso_cache.get());
    m_globals->put(m_rs_cache.get());
}


}
