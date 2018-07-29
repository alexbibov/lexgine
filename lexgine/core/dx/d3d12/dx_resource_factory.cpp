#include "dx_resource_factory.h"
#include "debug_interface.h"

using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


DxResourceFactory::DxResourceFactory(GlobalSettings const& global_settings, 
    bool enable_debug_mode,
    dxgi::HwAdapterEnumerator::DxgiGpuPreference enumeration_preference) :
    m_global_settings{ global_settings },
    m_hw_adapter_enumerator{enable_debug_mode, enumeration_preference},
    m_dxc_proxy(m_global_settings),
    m_debug_interface{ enable_debug_mode ? DebugInterface::retrieve() : nullptr }
{
}

dxgi::HwAdapterEnumerator const& DxResourceFactory::hardwareAdapterEnumerator() const
{
    return m_hw_adapter_enumerator;
}

dxcompilation::DXCompilerProxy& DxResourceFactory::shaderModel6xDxCompilerProxy()
{
    return m_dxc_proxy;
}

DebugInterface const* lexgine::core::dx::d3d12::DxResourceFactory::debugInterface() const
{
    return m_debug_interface;
}
