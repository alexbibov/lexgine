#ifndef LEXGINE_INITIALIZER_H
#define LEXGINE_INITIALIZER_H

#include <string>
#include <memory>
#include <cstdint>


#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"
#include "engine/core/dx/lexgine_core_dx_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/dxgi/interface.h"
#include "engine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"

#include "engine/core/swap_chain_desc.h"
#include "engine/preprocessing/preprocessor_tokens.h"
#include "engine/osinteraction/window_handler.h"
#include "engine/osinteraction/windows/window.h"
#include "engine/conversion/lexgine_conversion_fwd.h"

namespace lexgine {


struct LEXGINE_CPP_API EngineSettings
{
    core::EngineApi engine_api;
    bool debug_mode;
    bool enable_profiling;
    std::string global_lookup_prefix;
    std::string settings_lookup_path;
    std::string global_settings_json_file;
    std::string logging_output_path;
    std::string log_name;

    EngineSettings();
};

struct LEXGINE_CPP_API DeviceDetails
{
    std::wstring description;
    size_t dedicated_video_memory;
    size_t dedicated_system_memory;
    size_t shared_system_memory;
    core::dx::dxgi::D3D12FeatureLevel d3d12_feature_level;
};


class LEXGINE_CPP_API DEPENDS_ON(EngineSettings, DeviceDetails) Initializer
{
public:
    Initializer(EngineSettings const& settings);
    ~Initializer();

    LEXGINE_CPP_API bool setCurrentDevice(uint32_t adapter_id);    //! assigns device that will be used for graphics and compute tasks. Returns 'true' on success
    LEXGINE_CPP_API uint32_t getAdapterCount() const;    //! returns total number of adapters installed in the host system including possibly available software devices
    LEXGINE_CPP_API DeviceDetails getDeviceDetails() const;    //! retrieves detailed information about currently active device

    LEXGINE_CPP_API osinteraction::WindowHandler* createWindowHandler(osinteraction::windows::Window & window, core::SwapChainDescriptor const& swap_chain_desc, core::SwapChainDepthFormat depth_format);

    core::Globals& globals();

private:
    core::EngineApi m_engine_api;
    std::unique_ptr<core::dx::D3D12Initializer> m_d3d12_initializer;
    std::unique_ptr<core::dx::dxgi::SwapChain> m_swap_chain;
    std::unique_ptr<core::dx::d3d12::RenderingTasks> m_rendering_tasks;
    std::shared_ptr<core::dx::d3d12::SwapChainLink> m_swap_chain_link;
    std::unique_ptr<osinteraction::WindowHandler> m_window_handler;
    std::unique_ptr<conversion::ImageLoaderPool> m_image_loader_pool;
};

}

#endif
