#include <algorithm>

#include "debug_interface.h"
#include "device.h"
#include "descriptor_heap.h"
#include "frame_progress_tracker.h"

#include "dx_resource_factory.h"


using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


DxResourceFactory::DxResourceFactory(GlobalSettings const& global_settings,
    bool enable_debug_mode,
    dxgi::HwAdapterEnumerator::DxgiGpuPreference enumeration_preference)
    : m_global_settings{ global_settings }
    , m_debug_interface{ enable_debug_mode ? DebugInterface::retrieve() : nullptr }
    , m_hw_adapter_enumerator{ global_settings, enable_debug_mode, enumeration_preference }
    , m_dxc_proxy(m_global_settings)
{
    m_hw_adapter_enumerator.setStringName("Hardware adapter enumerator");

    for (auto& adapter : m_hw_adapter_enumerator)
    {
        Device& dev_ref = adapter.device();

        uint32_t node_mask = 1;

        // initialize descriptor heaps

        descriptor_heap_page_pool page_pool{};
        std::array<std::string, 4U> descriptor_heap_name_suffixes = {
            "_cbv_srv_uav_heap", "_sampler_heap", "_rtv_heap", "_dsv_heap"
        };
        for (auto heap_type :
            { DescriptorHeapType::cbv_srv_uav,
            DescriptorHeapType::sampler,
            DescriptorHeapType::rtv,
            DescriptorHeapType::dsv })
        {
            uint32_t descriptor_heap_page_count = global_settings.getDescriptorHeapPageCount(heap_type);
            page_pool[static_cast<size_t>(heap_type)].resize(descriptor_heap_page_count);

            for (uint32_t page_id = 0U; page_id < descriptor_heap_page_count; ++page_id)
            {
                uint32_t descriptor_count = global_settings.getDescriptorHeapPageCapacity(heap_type);
                auto& new_descriptor_heap_ref =
                    page_pool[static_cast<size_t>(heap_type)][page_id] =
                    dev_ref.createDescriptorHeap(heap_type, descriptor_count, node_mask);

                new_descriptor_heap_ref->setStringName(dev_ref.getStringName()
                    + descriptor_heap_name_suffixes[static_cast<size_t>(heap_type)] + "_page" + std::to_string(page_id));
            }
        }

        m_descriptor_heaps.emplace(&dev_ref, std::move(page_pool));

        // initialize upload heaps
        {
            Heap upload_heap = dev_ref.createHeap(AbstractHeapType::upload, global_settings.getUploadHeapCapacity(),
                HeapCreationFlags::enum_type::allow_only_buffers);
            upload_heap.setStringName(dev_ref.getStringName() + "__upload_heap");

            m_upload_heaps.emplace(&dev_ref, std::move(upload_heap));
        }


        // initialize frame progress trackers
        {
            m_frame_progress_trackers.emplace(&dev_ref, FrameProgressTracker{ dev_ref });
        }

    }
}

DxResourceFactory::~DxResourceFactory()
{
    if (m_debug_interface)
        m_debug_interface->shutdown();
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

DescriptorHeap& DxResourceFactory::retrieveDescriptorHeap(Device const& device, DescriptorHeapType descriptor_heap_type, uint32_t page_id)
{
    return *m_descriptor_heaps.at(&device)[static_cast<size_t>(descriptor_heap_type)][page_id];
}

Heap& DxResourceFactory::retrieveUploadHeap(Device const& device)
{
    return m_upload_heaps.at(&device);
}

FrameProgressTracker& DxResourceFactory::retrieveFrameProgressTracker(Device const& device)
{
    return m_frame_progress_trackers.at(&device);
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
