#include "init.h"

#include "engine/core/initializer.h"
#include "engine/core/dx/dxgi/swap_chain.h"
#include "engine/osinteraction/listener.h"

using namespace lexgine;
using namespace lexgine::runtime;
using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::dxgi;
using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;

#define LEXGINE_MAGIC_BYTE_OBFUSCATION 0x16A6552FDA56AE97 
#define PTR_OBFUSCATE(ptr) reinterpret_cast<void*>(reinterpret_cast<uint64_t>(ptr) | LEXGINE_MAGIC_BYTE_OBFUSCATION)

namespace {

struct EngineConfigurationInternal
{
    EngineSettings engine_settings;
    SwapChainDescriptor swap_chain_descriptor;
};


}


class EngineManager::impl final :
    public Listeners<
    KeyInputListener,
    MouseButtonListener,
    MouseMoveListener,
    WindowSizeChangeListener,
    ClientAreaUpdateListener,
    CursorUpdateListener>
{
private:
    EngineSettings m_engine_settings;

};


EngineManager::EngineManager(EngineConfiguration engine_configuration)
{
}

EngineConfiguration lexgine::runtime::createEngineConfiguration(EngineGeneralSettings const& settings)
{
    EngineConfigurationInternal* p_config = new EngineConfigurationInternal{};
    p_config->engine_settings.debug_mode = settings.debug_mode;
    p_config->engine_settings.enable_profiling = settings.enable_profiling;
    p_config->engine_settings.gpu_based_validation_settings = settings.gpu_based_validation;
    p_config->engine_settings.adapter_enumeration_preference = settings.prioritized_gpu;
    
    p_config->swap_chain_descriptor.refreshRate = settings.refresh_rate;
    p_config->swap_chain_descriptor.stereo = settings.stereo_back_buffer;
    p_config->swap_chain_descriptor.windowed = settings.windowed;
    p_config->swap_chain_descriptor.back_buffer_count = settings.back_buffer_count;
    p_config->swap_chain_descriptor.enable_vsync = settings.vsync_on;
    p_config->swap_chain_descriptor.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    p_config->swap_chain_descriptor.scaling = SwapChainScaling::none;


    return EngineConfiguration{ PTR_OBFUSCATE(p_config) };
}

void lexgine::runtime::destroyEngineConfiguration(EngineConfiguration engine_configuration)
{
    delete PTR_OBFUSCATE(engine_configuration->_ptr);
}


