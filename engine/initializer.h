#ifndef LEXGINE_INITIALIZER_H
#define LEXGINE_INITIALIZER_H

#include <string>
#include <memory>

#include "engine/preprocessing/preprocessor_tokens.h"
#include "engine/core/engine_api.h"
#include "engine/osinteraction/window_handler.h"
#include "engine/core/swap_chain.h"
#include "engine/core/dx/d3d12_initializer.h"

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

class LEXGINE_CPP_API DEPENDS_ON(EngineSettings) Initializer 
{
public:
    Initializer(EngineSettings const& settings);

    LEXGINE_CPP_API bool setCurrentDevice(uint32_t adapter_id);    //! assigns device that will be used for graphics and compute tasks. Returns 'true' on success
    LEXGINE_CPP_API uint32_t getAdapterCount() const;    //! returns total number of adapters installed in the host system including possibly available software devices
    
    //! helper function, which simplifies creation of a swap chain for the current device
    LEXGINE_CPP_API std::unique_ptr<lexgine::core::SwapChain> createSwapChainForCurrentDevice(osinteraction::WindowHandler& window_handler, core::SwapChainDescriptor const& descriptor) const;

private:
    core::EngineApi m_engine_api;
    std::unique_ptr<core::dx::D3D12Initializer> m_d3d12_initializer;
};

}

#endif
