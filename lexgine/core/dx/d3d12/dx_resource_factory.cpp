#include "dx_resource_factory.h"
#include "debug_interface.h"

#include "lexgine/core/dx/d3d12/device.h"

using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


DxResourceFactory::DxResourceFactory(GlobalSettings const& global_settings, 
    bool enable_debug_mode,
    dxgi::HwAdapterEnumerator::DxgiGpuPreference enumeration_preference) :
    m_global_settings{ global_settings },
    m_debug_interface{ enable_debug_mode ? DebugInterface::retrieve() : nullptr },
    m_hw_adapter_enumerator{global_settings, enable_debug_mode, enumeration_preference},
    m_dxc_proxy(m_global_settings)
{
    m_hw_adapter_enumerator.setStringName("Hardware adapter enumerator");

    uint32_t descriptor_heaps_num_pages = global_settings.getDescriptorPageCount();

    for (auto& adapter : m_hw_adapter_enumerator)
    {
        Device& dev_ref = adapter.device();

        uint32_t node_mask = 1;

        // initialize descriptor heaps
        {
            descriptor_heap_page_pool page_pool{};
            for (uint32_t page_id = 0U; page_id < descriptor_heaps_num_pages; ++page_id)
            {
                uint32_t num_descriptors = global_settings.getDescriptorPageCapacity(page_id);

                std::array<DescriptorHeap, 4> descriptor_heaps = {
                    dev_ref.createDescriptorHeap(DescriptorHeapType::cbv_srv_uav, num_descriptors, node_mask),
                    dev_ref.createDescriptorHeap(DescriptorHeapType::sampler, num_descriptors, node_mask),
                    dev_ref.createDescriptorHeap(DescriptorHeapType::rtv, num_descriptors, node_mask),
                    dev_ref.createDescriptorHeap(DescriptorHeapType::dsv, num_descriptors, node_mask)
                };

                descriptor_heaps[0].setStringName(dev_ref.getStringName() + "__cbv_srv_uav_heap");
                descriptor_heaps[1].setStringName(dev_ref.getStringName() + "__sampler_heap");
                descriptor_heaps[2].setStringName(dev_ref.getStringName() + "__rtv_heap");
                descriptor_heaps[3].setStringName(dev_ref.getStringName() + "__dsv_heap");

                page_pool.push_back(std::move(descriptor_heaps));
            }

            m_descriptor_heaps.emplace(&dev_ref, std::move(page_pool));
        }

        // initialize upload heaps
        {
            Heap upload_heap = dev_ref.createHeap(AbstractHeapType::upload, global_settings.getUploadHeapCapacity(),
                HeapCreationFlags::enum_type::allow_only_buffers);
            upload_heap.setStringName(dev_ref.getStringName() + "__upload_heap");

            m_upload_heaps.emplace(&dev_ref, std::move(upload_heap));
        }

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

DescriptorHeap& DxResourceFactory::retrieveDescriptorHeap(Device const& device, uint32_t page_id, DescriptorHeapType descriptor_heap_type)
{
    return m_descriptor_heaps.at(&device)[page_id][static_cast<size_t>(descriptor_heap_type)];
}

Heap& DxResourceFactory::retrieveUploadHeap(Device const& device)
{
    return m_upload_heaps.at(&device);
}

dxgi::HwAdapter const* DxResourceFactory::retrieveHwAdapterOwningDevicePtr(Device const& device) const
{
    auto target_adapter = std::find_if(m_hw_adapter_enumerator.begin(), m_hw_adapter_enumerator.end(),
        [&device](auto& adapter)
        {
            Device& dev_ref = adapter.device();
            return &dev_ref == &device;
        }
    );

    if (target_adapter != m_hw_adapter_enumerator.end())
    {
        dxgi::HwAdapter const& adapter_ref = *target_adapter;
        return &adapter_ref;
    }
    else
        return nullptr;
}
