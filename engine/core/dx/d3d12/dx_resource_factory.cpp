#include <algorithm>

#include "engine/core/exception.h"

#include "device.h"
#include "descriptor_heap.h"
#include "static_descriptor_allocation_manager.h"

#include "dx_resource_factory.h"


using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


DxResourceFactory::DxResourceFactory(GlobalSettings const& global_settings,
    bool enable_debug_mode, GpuBasedValidationSettings const& gpu_based_validation_settings,
    dxgi::DxgiGpuPreference enumeration_preference)
    : m_global_settings{ (enable_debug_mode ? DebugInterface::create(gpu_based_validation_settings) : nullptr, global_settings) }
    , m_hw_adapter_enumerator{ global_settings, enumeration_preference }
    , m_dxc_proxy(m_global_settings)
{
    m_hw_adapter_enumerator.setStringName("Hardware adapter enumerator");

    for (auto& adapter : m_hw_adapter_enumerator)
    {
        Device& dev_ref = adapter->device();

        uint32_t node_mask = 1;

        // initialize descriptor heaps

        descriptor_heap_page_pool page_pool{};
        std::array<std::string, descriptor_heap_page_pool::heap_type_count> descriptor_heap_name_suffixes = {
            "_cbv_srv_uav_heap", "_sampler_heap", "_rtv_heap", "_dsv_heap"
        };
        for (auto heap_type :
            { DescriptorHeapType::cbv_srv_uav,
            DescriptorHeapType::sampler,
            DescriptorHeapType::rtv,
            DescriptorHeapType::dsv })
        {
            uint32_t descriptor_heap_page_count = global_settings.getDescriptorHeapPageCount(heap_type);
            page_pool.heaps[static_cast<size_t>(heap_type)].resize(descriptor_heap_page_count);
            page_pool.static_allocators[static_cast<size_t>(heap_type)].resize(descriptor_heap_page_count);

            for (uint32_t page_id = 0U; page_id < descriptor_heap_page_count; ++page_id)
            {
                uint32_t descriptor_count = global_settings.getDescriptorHeapPageCapacity(heap_type);
                auto& new_descriptor_heap_ref =
                    page_pool.heaps[static_cast<size_t>(heap_type)][page_id] = 
                    dev_ref.createDescriptorHeap(heap_type, descriptor_count, node_mask);

                new_descriptor_heap_ref->setStringName(dev_ref.getStringName()
                    + descriptor_heap_name_suffixes[static_cast<size_t>(heap_type)] + "_page" + std::to_string(page_id));

                page_pool.static_allocators[static_cast<size_t>(heap_type)][page_id] =
                    std::make_unique<StaticDescriptorAllocationManager>(*new_descriptor_heap_ref);
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
    return *m_descriptor_heaps.at(&device).heaps[static_cast<size_t>(descriptor_heap_type)][page_id];
}

Heap& DxResourceFactory::retrieveUploadHeap(Device const& device)
{
    return m_upload_heaps.at(&device);
}

StaticDescriptorAllocationManager& lexgine::core::dx::d3d12::DxResourceFactory::getStaticAllocationManagerForDescriptorHeap(Device const& device, DescriptorHeapType descriptor_heap_type, uint32_t page_id)
{
    return *m_descriptor_heaps.at(&device).static_allocators[static_cast<size_t>(descriptor_heap_type)][page_id];
}


misc::Optional<UploadHeapPartition> DxResourceFactory::allocateSectionInUploadHeap(Heap const& upload_heap, std::string const& section_name, size_t section_size)
{
    size_t aligned_section_size = misc::align(section_size, 1 << 16);

    auto p = m_upload_heap_partitions.find(&upload_heap);
    if (p == m_upload_heap_partitions.end())
    {
        upload_heap_partitioning new_partitioning{};
        new_partitioning.partitioned_space_size = aligned_section_size;
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

            if (offset + aligned_section_size <= upload_heap.capacity())
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
        else 
        {
            return q->second;
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

size_t DxResourceFactory::getUploadHeapFreeSpace(Heap const& upload_heap) const
{
    auto p = m_upload_heap_partitions.find(&upload_heap);
    size_t upload_heap_full_capacity = m_global_settings.getUploadHeapCapacity();
    return p != m_upload_heap_partitions.end() ? upload_heap_full_capacity - p->second.partitioned_space_size : 0;
}

size_t DxResourceFactory::getUploadHeapFreeSpace(Device const& owning_device) const
{
    auto p = m_upload_heaps.find(&owning_device);
    return p != m_upload_heaps.end() ? getUploadHeapFreeSpace(p->second) : 0;
}