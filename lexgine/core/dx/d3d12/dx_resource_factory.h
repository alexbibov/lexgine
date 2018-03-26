#ifndef LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H
#define LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H

#include "lexgine/core/global_settings.h"

namespace lexgine { namespace core { namespace dx { namespace d3d12{

//! Used to create and encapsulate reused Direct3D resources
class DxResourceFactory
{
public:
    DxResourceFactory(GlobalSettings const& global_settings);

private:
    GlobalSettings const& m_global_settings;
};

}}}}


#endif
