#ifndef LEXGINE_RUNTIME_INIT_H
#define LEXGINE_RUNTIME_INIT_H

#define LEXGINE_API __declspec(dllexport)

#include <engine/core/initializer.h>
#include <engine/core/dx/d3d12/debug_interface.h>
#include <engine/core/dx/dxgi/hw_adapter_enumerator.h>

namespace lexgine::runtime {

// **************************************************** Engine configuration routines ****************************************************

//! Opaque type, which determines configuration of the engine 
struct _EngineConfiguration
{
    void* _ptr;
};

using EngineConfiguration = _EngineConfiguration const*;
using PreferredGPU = core::dx::dxgi::HwAdapterEnumerator::DxgiGpuPreference;
using GpuBasedValidation = core::dx::d3d12::GpuBasedValidationSettings;


//! Creates basic engine configuration
LEXGINE_API EngineConfiguration createEngineConfiguration(
    bool enable_profiling = false, 
    bool debug_mode = false, 
    PreferredGPU preferred_gpu = PreferredGPU::high_performance
);


//! Configures engine path look up prefix
LEXGINE_API void configureLookUpPathPrefix(EngineConfiguration engine_configuration, std::string const& prefix_string);

//! Configures engine global settings path
LEXGINE_API void configureSettingsLookUpPath(EngineConfiguration engine_configuration, std::string const& path);


//! Configures logging output path
LEXGINE_API void configureLoggingPath(EngineConfiguration engine_configuration, std::string const& path);


//! Manages main engine entities (rendering output window, currently used device, swap-chain, etc.)
class LEXGINE_API EngineManager 
{
public:
    EngineManager(EngineConfiguration engine_configuration);
};


// ***************************************************************************************************************************************
}


#endif