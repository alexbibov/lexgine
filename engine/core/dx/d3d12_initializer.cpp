#include <system_error>

#include "d3d12_initializer.h"

#include "engine/core/globals.h"
#include "engine/core/global_settings.h"

#include "engine/core/misc/log.h"

#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"
#include "engine/core/dx/d3d12/caches/pso_blob_cache.h"

#include "engine/core/gpu_data_blob_cache.h"

#include "engine/conversion/texture_converter.h"


using namespace lexgine;
using namespace lexgine::core;
namespace lexgine::core::dx {


D3D12EngineSettings::D3D12EngineSettings()
    : debug_mode{ false }
    , enable_dred{ false }
    , enable_profiling{ false }
    , msaa_mode{ MSAAMode::none }
    , adapter_enumeration_preference{ dx::dxgi::DxgiGpuPreference::high_performance }
    , global_settings_json_file{ "global_settings.json" }
{

}


D3D12Initializer::D3D12Initializer(Globals& globals, D3D12EngineSettings const& settings)
    : m_engine_globals{ globals }
    , m_debug_mode{ settings.debug_mode }
    , m_enable_dred{ settings.enable_dred }
    , m_gpu_based_validation_settings{ settings.gpu_based_validation_settings }
    , m_adapter_enumeration_preference{ settings.adapter_enumeration_preference }
{
    // Load the global settings
    auto global_settings = std::make_unique<GlobalSettings>(settings.global_lookup_prefix / settings.settings_lookup_path / settings.global_settings_json_file);

    // Correct shader lookup directories
	{
		std::vector<std::filesystem::path> new_shader_lookup_directories;
		for (auto& shader_lookup_path : global_settings->getShaderLookupDirectories())
		{
			new_shader_lookup_directories.push_back(settings.global_lookup_prefix / shader_lookup_path);
		}
		global_settings->clearShaderLookupDirectories();
		for (auto& new_shader_lookup_path : new_shader_lookup_directories)
		{
			global_settings->addShaderLookupDirectory(new_shader_lookup_path);
		}
		global_settings->addShaderLookupDirectory(settings.global_lookup_prefix);
	}

    // Correct cache path
    {
        std::filesystem::path corrected_cache_path = settings.global_lookup_prefix / global_settings->getCacheDirectory();
        global_settings->setCacheDirectory(corrected_cache_path);

        std::error_code ec;
        std::filesystem::create_directories(corrected_cache_path, ec);
        if (ec)
        {
            misc::Log::retrieve()->out("Unable to create cache path \"" + corrected_cache_path.string() + "\". Caching may not function properly, which "
                "may deteriorate performance of the system", misc::LogMessageType::exclamation);
        }
    }

    // Set profiling enable state
    global_settings->setIsProfilingEnabled(settings.enable_profiling);

    auto d3d12_globals = std::make_unique<D3D12Globals>();
    d3d12_globals->m_global_settings = std::move(global_settings);
    d3d12_globals->m_dx_resource_factory =
        std::make_unique<d3d12::DxResourceFactory>(d3d12_globals->globalSettings(), m_debug_mode, m_enable_dred,
            m_gpu_based_validation_settings, m_adapter_enumeration_preference);
    d3d12_globals->m_gpu_data_blob_cache = std::make_unique<GpuDataBlobCache>(d3d12_globals->globalSettings());
    GlobalsAttorney<D3D12Initializer>::setD3D12Globals(m_engine_globals, std::move(d3d12_globals));

    setCurrentDevice(0);
}

bool D3D12Initializer::setCurrentDevice(uint32_t adapter_id)
{
    auto& hw_adapter_enumerator = m_engine_globals.d3d12().dxResourceFactory().hardwareAdapterEnumerator();
    if (adapter_id >= hw_adapter_enumerator.getAdapterCount()) return false;

    selectDevice(hw_adapter_enumerator[adapter_id]->device());
    return true;
}

bool D3D12Initializer::recreateDevice(LUID const& adapter_luid)
{
    auto& d3d12_globals = m_engine_globals.d3d12();
    d3d12_globals.releaseDeviceDependentEntries();
    d3d12_globals.m_dx_resource_factory =
        std::make_unique<d3d12::DxResourceFactory>(d3d12_globals.globalSettings(), m_debug_mode, m_enable_dred,
            m_gpu_based_validation_settings, m_adapter_enumeration_preference);

    auto& hw_adapter_enumerator = d3d12_globals.dxResourceFactory().hardwareAdapterEnumerator();
    uint32_t const adapter_count = hw_adapter_enumerator.getAdapterCount();
    for (uint32_t adapter_id = 0; adapter_id < adapter_count; ++adapter_id)
    {
        LUID const luid = hw_adapter_enumerator[adapter_id]->getProperties().details.luid;
        if (luid.LowPart == adapter_luid.LowPart && luid.HighPart == adapter_luid.HighPart)
        {
            return setCurrentDevice(adapter_id);
        }
    }

    misc::Log::retrieve()->out("The adapter of the lost device is no longer present, the first available adapter "
        "is used instead", misc::LogMessageType::exclamation);
    return setCurrentDevice(0);
}

dx::d3d12::Device& D3D12Initializer::getCurrentDevice() const
{
    return m_engine_globals.d3d12().device();
}

void D3D12Initializer::setWARPAdapterAsCurrent() const
{
    auto& adapter_enumerator = m_engine_globals.d3d12().dxResourceFactory().hardwareAdapterEnumerator();
    selectDevice(adapter_enumerator.getWARPAdapter()->device());
}

uint32_t D3D12Initializer::getAdapterCount() const
{
    return m_engine_globals.d3d12().dxResourceFactory().hardwareAdapterEnumerator().getAdapterCount();
}

dx::dxgi::SwapChain D3D12Initializer::createSwapChainForCurrentDevice(osinteraction::windows::Window& window, dx::dxgi::SwapChainDescriptor const& desc) const
{
    return getCurrentDevice().hwAdapter()->createSwapChain(window, desc);
}

std::unique_ptr<dx::d3d12::RenderingTasks> D3D12Initializer::createRenderingTasks(osinteraction::windows::Window& rendering_window,
    std::function<bool(uint32_t)> adapter_selector) const
{
    return std::make_unique<dx::d3d12::RenderingTasks>(m_engine_globals, rendering_window, std::move(adapter_selector));
}

std::shared_ptr<dx::d3d12::SwapChainLink> D3D12Initializer::createSwapChainLink(dx::dxgi::SwapChain& target_swap_chain,
    dx::d3d12::SwapChainDepthBufferFormat depth_buffer_format,
    dx::d3d12::RenderingTasks& source_rendering_tasks) const
{
    auto rv = dx::d3d12::SwapChainLink::create(m_engine_globals, target_swap_chain, depth_buffer_format);
    rv->linkRenderingTasks(&source_rendering_tasks);
    return rv;
}

void D3D12Initializer::selectDevice(d3d12::Device& device) const
{
    auto& d3d12_globals = m_engine_globals.d3d12();
    d3d12_globals.resetDeviceDependentCaches();
    d3d12_globals.m_texture_converter.reset();
    d3d12_globals.m_device = &device;
    d3d12_globals.m_texture_converter = std::make_unique<conversion::TextureConverter>(m_engine_globals);
    d3d12_globals.m_hlsl_shader_blob_cache = std::make_unique<d3d12::caches::HLSLShaderBlobCache>(m_engine_globals);
    d3d12_globals.m_root_signature_blob_cache = std::make_unique<d3d12::caches::RootSignatureBlobCache>(m_engine_globals);
    d3d12_globals.m_pso_blob_cache = std::make_unique<d3d12::caches::PSOBlobCache>(m_engine_globals);
}


}
