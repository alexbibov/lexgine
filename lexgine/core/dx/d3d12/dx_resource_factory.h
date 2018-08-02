#ifndef LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H
#define LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H

#include "lexgine/core/global_settings.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/dxgi/hw_adapter_enumerator.h"
#include "lexgine/core/dx/dxcompilation/dx_compiler_proxy.h"
#include "lexgine/core/dx/d3d12/descriptor_heap.h"

#include <deque>

namespace lexgine { namespace core { namespace dx { namespace d3d12{

//! Used to create and encapsulate reused Direct3D resources
class DxResourceFactory final
{
public:
    DxResourceFactory(GlobalSettings const& global_settings,
        bool enable_debug_mode,
        dxgi::HwAdapterEnumerator::DxgiGpuPreference enumeration_preference);

    ~DxResourceFactory();

    dxgi::HwAdapterEnumerator const& hardwareAdapterEnumerator() const;
    dxcompilation::DXCompilerProxy& shaderModel6xDxCompilerProxy();
    DebugInterface const* debugInterface() const;
    
    DescriptorHeap& retrieveDescriptorHeap(Device const& device, uint32_t page_id, DescriptorHeapType descriptor_heap_type);

private:
    using descriptor_heap_page_pool = std::deque<std::array<DescriptorHeap, 4U>>;

private:
    GlobalSettings const& m_global_settings;
    dx::d3d12::DebugInterface const* m_debug_interface;
    dxgi::HwAdapterEnumerator m_hw_adapter_enumerator;
    dxcompilation::DXCompilerProxy m_dxc_proxy;
    std::map<Device const*, descriptor_heap_page_pool> m_descriptor_heaps;
};

}}}}


#endif
