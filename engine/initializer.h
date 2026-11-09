#ifndef LEXGINE_INITIALIZER_H
#define LEXGINE_INITIALIZER_H

#include <string>
#include <memory>
#include <mutex>
#include <optional>
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
    bool enable_dred;    //!< records device removed extended data and reports it on device loss; only effective when debug_mode is on
    core::MSAAMode msaa_mode;
    std::string global_lookup_prefix;
    std::string settings_lookup_path;
    std::string shaders_lookup_path;
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

    /*! makes current the device of the adapter identified by @param adapter_id, as the adapters are enumerated at the
     time of the call. Before createWindowHandler() the device is switched immediately and the function may only be
     called from the simulation thread. Afterwards the switch is scheduled: the device and the rendering chain are
     rebuilt on the simulation thread during the next update of the window handler and the current scene is reloaded;
     the function may then be called from the simulation thread or from any rendering thread. Returns 'false' if there
     is no adapter with the given identifier or the immediate switch has failed
    */
    LEXGINE_CPP_API bool setCurrentDevice(uint32_t adapter_id);
    LEXGINE_CPP_API uint32_t getAdapterCount() const;    //! returns total number of adapters installed in the host system including possibly available software devices
    LEXGINE_CPP_API DeviceDetails getDeviceDetails() const;    //! retrieves detailed information about currently active device

    LEXGINE_CPP_API osinteraction::WindowHandler* createWindowHandler(osinteraction::windows::Window & window, core::SwapChainDescriptor const& swap_chain_desc, core::SwapChainDepthFormat depth_format);

    /*! stops rendering and destroys the rendering chain built by createWindowHandler(): the thread driving the
     frames, the swap chain and the rendering tasks. Must be called from the simulation thread while the window
     passed to createWindowHandler() is still alive; the destructor calls it if the client has not. Requests to
     switch the device made afterwards are rejected. Calling the function repeatedly has no effect
    */
    LEXGINE_CPP_API void shutdown();

    core::Globals& globals();

private:
    void buildRenderingChain();    //! creates the swap chain, the rendering tasks and the thread driving the frames over the current device
    void releaseRenderingChain();    //! destroys everything built by buildRenderingChain() in the reverse order

    //! rebuilds the device and the rendering chain over it after the device has been lost or an adapter switch requested
    void handleDeviceRecreationRequest();

private:
    core::EngineApi m_engine_api;
    std::unique_ptr<core::dx::D3D12Initializer> m_d3d12_initializer;
    std::unique_ptr<core::dx::dxgi::SwapChain> m_swap_chain;
    std::unique_ptr<core::dx::d3d12::RenderingTasks> m_rendering_tasks;
    std::shared_ptr<core::dx::d3d12::SwapChainLink> m_swap_chain_link;
    std::unique_ptr<osinteraction::WindowHandler> m_window_handler;

    osinteraction::windows::Window* m_rendering_window_ptr = nullptr;
    core::SwapChainDescriptor m_swap_chain_descriptor{};
    core::SwapChainDepthFormat m_swap_chain_depth_format{};

    /*!< signalled when the device has to be rebuilt: by the rendering thread once the device is lost, or by
     setCurrentDevice() once the rendering chain exists
    */
    HANDLE m_device_recreation_event = nullptr;
    std::mutex m_device_recreation_mutex;
    std::optional<LUID> m_requested_adapter_luid;    //!< guarded by m_device_recreation_mutex
    bool m_is_shut_down = false;    //!< 'true' once shutdown() has destroyed the rendering chain

    /*!< declared last so that it is destroyed first: the thread has to leave its loop before the swap chain
     link and the rendering tasks it drives are torn down
    */
    std::shared_ptr<core::rendering::RenderThread> m_render_thread;
};

}

#endif
