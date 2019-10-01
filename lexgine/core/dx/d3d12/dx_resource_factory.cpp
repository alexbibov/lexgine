#include <algorithm>

#include "lexgine/core/exception.h"

#include "device.h"
#include "descriptor_heap.h"

#include "dx_resource_factory.h"


using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


DxResourceFactory::DxResourceFactory(GlobalSettings const& global_settings,
    bool enable_debug_mode, GpuBasedValidationSettings const& gpu_based_validation_settings,
    dxgi::HwAdapterEnumerator::DxgiGpuPreference enumeration_preference)
    : m_global_settings{ (enable_debug_mode ? DebugInterface::create(gpu_based_validation_settings) : nullptr, global_settings) }
    , m_hw_adapter_enumerator{ global_settings, enumeration_preference }
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
                HeapCreationFlags::base_values::allow_only_buffers);
            upload_heap.setStringName(dev_ref.getStringName() + "__upload_heap");

            m_upload_heaps.emplace(&dev_ref, std::move(upload_heap));
        }

    }
}

DxResourceFactory::~DxResourceFactory()
{
    DebugInterface::shutdown();
}

dxgi::HwAdapterEnumerator const& DxResourceFactory::hardwareAdapterEnumerator() const
{
    return m_hw_adapter_enumerator;
}

dxcompilation::DXCompilerProxy& DxResourceFactory::shaderModel6xDxCompilerProxy()
{
    return m_dxc_proxy;
}

DescriptorHeap& DxResourceFactory::retrieveDescriptorHeap(Device const& device, DescriptorHeapType descriptor_heap_type, uint32_t page_id)
{
    return *m_descriptor_heaps.at(&device)[static_cast<size_t>(descriptor_heap_type)][page_id];
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


misc::Optional<UploadHeapPartition> DxResourceFactory::allocateSectionInUploadHeap(Heap const& upload_heap, std::string const& section_name, size_t section_size)
{
    size_t aligned_section_size = misc::align(section_size, 1 << 16);

    auto p = m_upload_heap_partitions.find(&upload_heap);
    if (p == m_upload_heap_partitions.end())
    {
        upload_heap_partitioning new_partitioning{};
        new_partitioning.partitioned_space_size = section_size;
        auto q = new_partitioning.partitioning.insert(std::make_pair(
            misc::HashedString{ section_name },
            UploadHeapPartition{ 0ULL, aligned_section_size }
        )).first;

        m_upload_heap_partitions.insert(std::make_pair(&upload_heap, new_partitioning));
        return q->second;
    }
    else
    {
        misc::HashedString section_hash{ section_name };
        auto q = p->second.partitioning.find(section_hash);
        if (q == p->second.partitioning.end())
        {
            size_t& offset = p->second.partitioned_space_size;

            if(offset + section_size <= upload_heap.capacity())
            {
                auto r = p->second.partitioning.insert(std::make_pair(section_hash, UploadHeapPartition{ offset, aligned_section_size })).first;
                offset += aligned_section_size;
                return r->second;
            }
            else
            {
                LEXGINE_THROW_ERROR("Unable to allocated named section \"" + section_name
                    + "\" in upload heap \"" + upload_heap.getStringName() + "\": the heap is exhausted");
            }
        }
    }

    return misc::makeEmptyOptional<UploadHeapPartition>();
}

misc::Optional<UploadHeapPartition> DxResourceFactory::retrieveUploadHeapSection(Heap const& upload_heap, std::string const& section_name) const
{
    auto p = m_upload_heap_partitions.find(&upload_heap);
    if (p != m_upload_heap_partitions.end())
    {
        auto q = p->second.partitioning.find(misc::HashedString{ section_name });
        if (q != p->second.partitioning.end())
            return q->second;
    }

    return misc::makeEmptyOptional<UploadHeapPartition>();
}

