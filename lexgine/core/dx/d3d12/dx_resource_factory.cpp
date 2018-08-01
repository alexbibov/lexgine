#include "dx_resource_factory.h"
#include "debug_interface.h"

using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


DxResourceFactory::DxResourceFactory(GlobalSettings const& global_settings, 
    bool enable_debug_mode,
    dxgi::HwAdapterEnumerator::DxgiGpuPreference enumeration_preference) :
    m_global_settings{ global_settings },
    m_debug_interface{ enable_debug_mode ? DebugInterface::retrieve() : nullptr },
    m_hw_adapter_enumerator{enable_debug_mode, enumeration_preference},
    m_dxc_proxy(m_global_settings)
{
    uint32_t descriptor_heaps_capacity = global_settings.getDescriptorHeapsCapacity();

    for (auto& adapter : m_hw_adapter_enumerator)
    {
        Device& dev_ref = adapter.device();
        uint32_t node_mask = 1;

        descriptor_heap_dictionary descriptor_heaps = {
            dev_ref.createDescriptorHeap(DescriptorHeapType::cbv_srv_uav, descriptor_heaps_capacity, node_mask),
            dev_ref.createDescriptorHeap(DescriptorHeapType::sampler, descriptor_heaps_capacity, node_mask),
            dev_ref.createDescriptorHeap(DescriptorHeapType::rtv, descriptor_heaps_capacity, node_mask),
            dev_ref.createDescriptorHeap(DescriptorHeapType::dsv, descriptor_heaps_capacity, node_mask)
        };

        m_descriptor_heaps.emplace(&dev_ref, std::move(descriptor_heaps));
    }
}

DxResourceFactory::~DxResourceFactory()
{
    if (m_debug_interface) m_debug_interface->shutdown();
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
