#ifndef LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H
#define LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H

#include "lexgine/core/global_settings.h"
#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/dx/dxgi/hw_adapter_enumerator.h"

namespace lexgine { namespace core { namespace dx { namespace d3d12{

//! Used to create and encapsulate reused Direct3D resources
class DxResourceFactory
{
public:
    DxResourceFactory(GlobalSettings const& global_settings);

    dxgi::HwAdapterEnumerator& hardwareAdapterEnumerator() const;

private:
    GlobalSettings const& m_global_settings;
    dxgi::HwAdapterEnumerator m_dw_adapter_enumerator;
};

}}}}


#endif
