#include "dx_resource_factory.h"


using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


DxResourceFactory::DxResourceFactory(GlobalSettings const& global_settings):
    m_global_settings{ global_settings },
    m_dxc_proxy(m_global_settings)
{
}

dxgi::HwAdapterEnumerator const& DxResourceFactory::hardwareAdapterEnumerator() const
{
    return m_hw_adapter_enumerator;
}

dxcompilation::DXCompilerProxy& DxResourceFactory::RetrieveSM6DxCompilerProxy() const
{
    return m_dxc_proxy;
}

