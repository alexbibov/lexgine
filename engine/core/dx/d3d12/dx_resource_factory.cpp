#include <algorithm>

#include "engine/core/exception.h"

#include "device.h"
#include "descriptor_heap.h"

#include "dx_resource_factory.h"


namespace lexgine::core::dx::d3d12
{

char const* const DxResourceFactory::c_constant_data_section_name = "constant_data_section";
char const* const DxResourceFactory::c_dynamic_geometry_section_name = "dynamic_geometry_section";
char const* const DxResourceFactory::c_texture_section_name = "texture_section";

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

Heap& DxResourceFactory::retrieveUploadHeap(Device const& device)
{
    return m_upload_heaps.at(&device);
}

misc::Optional<UploadHeapPartition> DxResourceFactory::allocateSectionInUploadHeap(Heap const& upload_heap, std::string const& section_name, size_t section_size)
{
    size_t aligned_section_size = misc::align(section_size, 1 << 16);

    auto p = m_upload_heap_partitions.find(&upload_heap);
    if (p == m_upload_heap_partitions.end())
    {
        if (aligned_section_size <= upload_heap.capacity())
        {
            UploadHeapPartitionTable new_partitioning{};
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
            LEXGINE_THROW_ERROR(std::format("Unable to allocate named section {} in upload heap {}: requested {} bytes, when only {} is avaialble",
                section_name, upload_heap.getStringName(), aligned_section_size, upload_heap.capacity()));
        }
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
				LEXGINE_THROW_ERROR(std::format("Unable to allocate named section {} in upload heap {}: requested {} bytes, when only {} is avaialble",
					section_name, upload_heap.getStringName(), aligned_section_size, upload_heap.capacity() - p->second.partitioned_space_size));
            }
        }
        else
        {
            assert(q->second.size >= aligned_section_size);
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

}