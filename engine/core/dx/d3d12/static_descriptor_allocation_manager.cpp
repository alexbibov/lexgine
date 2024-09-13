#include <cassert>
#include <thread>

#include "descriptor_heap.h"

#include "static_descriptor_allocation_manager.h"

namespace lexgine::core::dx::d3d12 {

namespace
{

static std::unordered_map<DescriptorHeapType, float> const c_static_allocation_segment_portion{
    { DescriptorHeapType::cbv_srv_uav, 0.7f },
    { DescriptorHeapType::sampler, .9f },
    { DescriptorHeapType::rtv, .9f },
    { DescriptorHeapType::dsv, .9f }
};

size_t getStaticDescriptorHeapCapacity(DescriptorHeap const& descriptor_heap)
{
    return static_cast<size_t>(std::ceil(descriptor_heap.capacity() * c_static_allocation_segment_portion.at(descriptor_heap.type())));
}

}  // namespace

StaticDescriptorAllocationManager::StaticDescriptorAllocationManager(DescriptorHeap& descriptor_heap)
    : DescriptorAllocationManager{ descriptor_heap, getStaticDescriptorHeapCapacity(descriptor_heap) }
{
    size_t const descriptor_size = descriptor_heap.getDescriptorSize();
    switch (descriptor_heap.type())
    {
    case DescriptorHeapType::cbv_srv_uav:
    {
        m_descriptors_max_count.cbv_srv_uav.cbv_descriptors_max_count = (std::max)(static_cast<size_t>(std::floor(capacity() * c_cbv_portion)), 1ull);
        m_descriptors_max_count.cbv_srv_uav.srv_descriptors_max_count = (std::max)(static_cast<size_t>(std::ceil(capacity() * c_srv_portion)), 1ull);
        m_descriptors_max_count.cbv_srv_uav.uav_descriptors_max_count = (std::max)(static_cast<size_t>(std::ceil(capacity() * c_uav_portion)), 1ull);

        assert(descriptor_heap.type() != DescriptorHeapType::cbv_srv_uav
            || m_descriptors_max_count.cbv_srv_uav.cbv_descriptors_max_count
            + m_descriptors_max_count.cbv_srv_uav.srv_descriptors_max_count
            + m_descriptors_max_count.cbv_srv_uav.uav_descriptors_max_count <= capacity());
        assert(descriptor_heap.type() != DescriptorHeapType::cbv_srv_uav || m_descriptors_max_count.cbv_srv_uav.srv_descriptors_max_count >= 1000);
        assert(descriptor_heap.type() != DescriptorHeapType::cbv_srv_uav || m_descriptors_max_count.cbv_srv_uav.uav_descriptors_max_count >= 100);
        assert(descriptor_heap.type() != DescriptorHeapType::cbv_srv_uav || m_descriptors_max_count.cbv_srv_uav.cbv_descriptors_max_count >= 20);

        m_offset.cbv_srv_uav.cbv_descriptor_offset.store(descriptor_heap.reserveDescriptors(m_descriptors_max_count.cbv_srv_uav.cbv_descriptors_max_count), std::memory_order::memory_order_release);
        m_offset.cbv_srv_uav.srv_descriptor_offset.store(descriptor_heap.reserveDescriptors(m_descriptors_max_count.cbv_srv_uav.srv_descriptors_max_count), std::memory_order::memory_order_release);
        m_offset.cbv_srv_uav.uav_descriptor_offset.store(descriptor_heap.reserveDescriptors(m_descriptors_max_count.cbv_srv_uav.uav_descriptors_max_count), std::memory_order::memory_order_release);

        size_t base_offset = m_offset.cbv_srv_uav.cbv_descriptor_offset.load(std::memory_order::memory_order_acquire) * descriptor_size;
        m_descriptor_table.cbv_srv_uav.cpu_pointer = descriptor_heap.getBaseCPUPointer() + base_offset;
        m_base_gpu_address = m_descriptor_table.cbv_srv_uav.gpu_pointer = descriptor_heap.getBaseGPUPointer() + base_offset;
        m_descriptor_table.cbv_srv_uav.descriptor_count = capacity();
        m_descriptor_table.cbv_srv_uav.descriptor_size = descriptor_size;
        break;
    }

    case DescriptorHeapType::sampler:
    {
        destruct(DescriptorHeapType::cbv_srv_uav);
        construct(DescriptorHeapType::sampler);

        m_descriptors_max_count.sampler_descriptors_max_count = capacity();
        m_offset.sampler.store(descriptor_heap.reserveDescriptors(m_descriptors_max_count.sampler_descriptors_max_count), std::memory_order::memory_order_release);

        size_t base_offset = static_cast<uint64_t>(m_offset.sampler.load(std::memory_order::memory_order_acquire) * descriptor_size);
        m_descriptor_table.sampler.cpu_pointer = descriptor_heap.getBaseCPUPointer() + base_offset;
        m_base_gpu_address = m_descriptor_table.sampler.gpu_pointer = descriptor_heap.getBaseGPUPointer() + base_offset;
        m_descriptor_table.sampler.descriptor_count = capacity();
        m_descriptor_table.sampler.descriptor_size = descriptor_size;
        break;
    }

    case DescriptorHeapType::rtv:
    {
        destruct(DescriptorHeapType::cbv_srv_uav);
        construct(DescriptorHeapType::rtv);

        m_offset.rtv = {};
        m_offset.rtv.store(0, std::memory_order::memory_order_release);
        m_descritptor_cache.rtv_descriptors_lut = {};

        m_descriptors_max_count.rtv_descriptors_max_count = capacity();
        m_offset.rtv.store(descriptor_heap.reserveDescriptors(m_descriptors_max_count.rtv_descriptors_max_count), std::memory_order::memory_order_release);

        size_t base_offset = m_offset.rtv.load(std::memory_order::memory_order_acquire) * descriptor_size;
        m_descriptor_table.rtv.cpu_pointer = descriptor_heap.getBaseCPUPointer() + base_offset;
        m_base_gpu_address = m_descriptor_table.rtv.gpu_pointer = 0;
        m_descriptor_table.rtv.descriptor_count = capacity();
        m_descriptor_table.rtv.descriptor_size = descriptor_size;
        break;
    }

    case DescriptorHeapType::dsv:
    {
        destruct(DescriptorHeapType::cbv_srv_uav);
        construct(DescriptorHeapType::dsv);

        m_descriptors_max_count.dsv_descriptors_max_count = capacity();
        m_offset.dsv.store(descriptor_heap.reserveDescriptors(m_descriptors_max_count.dsv_descriptors_max_count), std::memory_order::memory_order_release);

        size_t dsv_address_offset = m_offset.rtv.load(std::memory_order::memory_order_acquire) * descriptor_size;
        m_descriptor_table.dsv.cpu_pointer = descriptor_heap.getBaseCPUPointer() + dsv_address_offset;
        m_base_gpu_address = m_descriptor_table.dsv.gpu_pointer = 0;
        m_descriptor_table.dsv.descriptor_count = capacity();
        m_descriptor_table.dsv.descriptor_size = descriptor_size;
        break;
    }

    default:
        LEXGINE_ASSUME;
    }
   
}

StaticDescriptorAllocationManager::~StaticDescriptorAllocationManager()
{
    destruct(heap().type());
}

misc::Optional<StaticDescriptorAllocationManager::UPointer> StaticDescriptorAllocationManager::getDescriptor(CBVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::cbv_srv_uav);

    return m_descritptor_cache.cbv_srv_uav.cbv_descriptors_lut.contains(desc) 
        ? misc::Optional<UPointer>{m_descritptor_cache.cbv_srv_uav.cbv_descriptors_lut[desc]} 
        : misc::Optional<UPointer>();
}

misc::Optional<StaticDescriptorAllocationManager::UPointer> StaticDescriptorAllocationManager::getDescriptor(SRVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::cbv_srv_uav);

    return m_descritptor_cache.cbv_srv_uav.srv_descriptors_lut.contains(desc)
        ? misc::Optional<UPointer> { m_descritptor_cache.cbv_srv_uav.srv_descriptors_lut[desc] }
        : misc::Optional<UPointer>();
}

misc::Optional<StaticDescriptorAllocationManager::UPointer> StaticDescriptorAllocationManager::getDescriptor(UAVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::cbv_srv_uav);

    return m_descritptor_cache.cbv_srv_uav.uav_descriptors_lut.contains(desc)
        ? misc::Optional<UPointer> { m_descritptor_cache.cbv_srv_uav.uav_descriptors_lut[desc] }
        : misc::Optional<UPointer>();
}

misc::Optional<StaticDescriptorAllocationManager::UPointer> StaticDescriptorAllocationManager::getDescriptor(SamplerDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::sampler);
    
    return m_descritptor_cache.sampler_descriptors_lut.contains(desc)
        ? misc::Optional<UPointer> { m_descritptor_cache.sampler_descriptors_lut[desc] }
        : misc::Optional<UPointer>();
}

misc::Optional<StaticDescriptorAllocationManager::UPointer> StaticDescriptorAllocationManager::getDescriptor(RTVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::rtv);
    
    return m_descritptor_cache.rtv_descriptors_lut.contains(desc)
        ? misc::Optional<UPointer> { m_descritptor_cache.rtv_descriptors_lut[desc] }
        : misc::Optional<UPointer>();
}


misc::Optional<StaticDescriptorAllocationManager::UPointer> StaticDescriptorAllocationManager::getDescriptor(DSVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::dsv);

    return m_descritptor_cache.dsv_descriptors_lut.contains(desc)
        ? misc::Optional<UPointer> { m_descritptor_cache.dsv_descriptors_lut[desc] }
        : misc::Optional<UPointer>();
}

StaticDescriptorAllocationManager::UPointer StaticDescriptorAllocationManager::getOrCreateDescriptor(CBVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::cbv_srv_uav);
    uint32_t old_offset = m_offset.cbv_srv_uav.cbv_descriptor_offset.fetch_add(1, std::memory_order::memory_order_acq_rel);
    auto [descriptor_pointer, result] = createDescriptor(old_offset, desc, &DescriptorHeap::createConstantBufferViewDescriptor);
    if (result) 
    {
        m_descritptor_cache.cbv_srv_uav.cbv_descriptors_lut[desc] = descriptor_pointer;
    }
    return descriptor_pointer;
}

StaticDescriptorAllocationManager::UPointer StaticDescriptorAllocationManager::getOrCreateDescriptor(SRVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::cbv_srv_uav);
    uint32_t old_offset = m_offset.cbv_srv_uav.srv_descriptor_offset.fetch_add(1, std::memory_order::memory_order_acq_rel);
    auto [descriptor_pointer, result] = createDescriptor(old_offset, desc, &DescriptorHeap::createShaderResourceViewDescriptor);
    if (result)
    {
        m_descritptor_cache.cbv_srv_uav.srv_descriptors_lut[desc] = descriptor_pointer;
    }
    return descriptor_pointer;
}

StaticDescriptorAllocationManager::UPointer StaticDescriptorAllocationManager::getOrCreateDescriptor(UAVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::cbv_srv_uav);
    uint32_t old_offset = m_offset.cbv_srv_uav.uav_descriptor_offset.fetch_add(1, std::memory_order::memory_order_acq_rel);
    auto [descriptor_pointer, result] = createDescriptor(old_offset, desc, &DescriptorHeap::createUnorderedAccessViewDescriptor);
    if (result) 
    {
        m_descritptor_cache.cbv_srv_uav.uav_descriptors_lut[desc] = descriptor_pointer;
    }
    return descriptor_pointer;
}

StaticDescriptorAllocationManager::UPointer StaticDescriptorAllocationManager::getOrCreateDescriptor(SamplerDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::sampler);
    uint32_t old_offset = m_offset.sampler.fetch_add(1, std::memory_order::memory_order_acq_rel);
    auto [descriptor_pointer, result] = createDescriptor(old_offset, desc, &DescriptorHeap::createSamplerDescriptor);
    if (result) 
    {
        m_descritptor_cache.sampler_descriptors_lut[desc] = descriptor_pointer;
    }
    return descriptor_pointer;
}

StaticDescriptorAllocationManager::UPointer StaticDescriptorAllocationManager::getOrCreateDescriptor(RTVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::rtv);
    uint32_t old_offset = m_offset.rtv.fetch_add(1, std::memory_order::memory_order_acq_rel);
    auto [descriptor_pointer, result] = createDescriptor(old_offset, desc, &DescriptorHeap::createRenderTargetViewDescriptor);
    if (result) 
    {
        m_descritptor_cache.rtv_descriptors_lut[desc] = descriptor_pointer;
    }
    return descriptor_pointer;
}

StaticDescriptorAllocationManager::UPointer StaticDescriptorAllocationManager::getOrCreateDescriptor(DSVDescriptor const& desc)
{
    assert(heap().type() == DescriptorHeapType::dsv);
    uint32_t old_offset = m_offset.dsv.fetch_add(1, std::memory_order::memory_order_acq_rel);
    auto [descriptor_pointer, result] = createDescriptor(old_offset, desc, &DescriptorHeap::createDepthStencilViewDescriptor);
    if (result) 
    {
        m_descritptor_cache.dsv_descriptors_lut[desc] = descriptor_pointer;
    }
    return descriptor_pointer;
}

void StaticDescriptorAllocationManager::construct(DescriptorHeapType heap_type)
{
    switch (heap_type)
    {
    case DescriptorHeapType::cbv_srv_uav:
        new (&m_offset.cbv_srv_uav.cbv_descriptor_offset) std::atomic_uint32_t{};
        new (&m_offset.cbv_srv_uav.srv_descriptor_offset) std::atomic_uint32_t{};
        new (&m_offset.cbv_srv_uav.uav_descriptor_offset) std::atomic_uint32_t{};

        m_offset.cbv_srv_uav.cbv_descriptor_offset.store(0, std::memory_order::memory_order_release);
        m_offset.cbv_srv_uav.srv_descriptor_offset.store(0, std::memory_order::memory_order_release);
        m_offset.cbv_srv_uav.uav_descriptor_offset.store(0, std::memory_order::memory_order_release);

        new (&m_descritptor_cache.cbv_srv_uav.cbv_descriptors_lut) std::unordered_map<CBVDescriptor, UPointer, DescriptorHasher<CBVDescriptor>>{};
        new (&m_descritptor_cache.cbv_srv_uav.srv_descriptors_lut) std::unordered_map<SRVDescriptor, UPointer, DescriptorHasher<SRVDescriptor>>{};
        new (&m_descritptor_cache.cbv_srv_uav.uav_descriptors_lut) std::unordered_map<UAVDescriptor, UPointer, DescriptorHasher<UAVDescriptor>>{};
        break;

    case DescriptorHeapType::sampler:
        new (&m_offset.sampler) std::atomic_uint32_t{};
        m_offset.sampler.store(0, std::memory_order::memory_order_release);
        new (&m_descritptor_cache.sampler_descriptors_lut) std::unordered_map<SamplerDescriptor, UPointer, DescriptorHasher<SamplerDescriptor>>{};
        break;

    case DescriptorHeapType::rtv:
        new (&m_offset.rtv) std::atomic_uint32_t{};
        m_offset.rtv.store(0, std::memory_order::memory_order_release);
        new (&m_descritptor_cache.rtv_descriptors_lut) std::unordered_map<RTVDescriptor, UPointer, DescriptorHasher<RTVDescriptor>>{};
        break;

    case DescriptorHeapType::dsv:
        new (&m_offset.dsv) std::atomic_uint32_t{};
        m_offset.dsv.store(0, std::memory_order::memory_order_release);
        new (&m_descritptor_cache.dsv_descriptors_lut) std::unordered_map<DSVDescriptor, UPointer, DescriptorHasher<DSVDescriptor>>{};
        break;
    
    default:
        LEXGINE_ASSUME;
    }
}

void StaticDescriptorAllocationManager::destruct(DescriptorHeapType heap_type)
{
    switch (heap_type)
    {
    case DescriptorHeapType::cbv_srv_uav:
        m_offset.cbv_srv_uav.cbv_descriptor_offset.~atomic();
        m_offset.cbv_srv_uav.srv_descriptor_offset.~atomic();
        m_offset.cbv_srv_uav.uav_descriptor_offset.~atomic();
        m_descritptor_cache.cbv_srv_uav.cbv_descriptors_lut.~unordered_map();
        m_descritptor_cache.cbv_srv_uav.srv_descriptors_lut.~unordered_map();
        m_descritptor_cache.cbv_srv_uav.uav_descriptors_lut.~unordered_map();
        break;

    case DescriptorHeapType::sampler:
        m_offset.sampler.~atomic();
        m_descritptor_cache.sampler_descriptors_lut.~unordered_map();
        break;

    case DescriptorHeapType::rtv:
        m_offset.rtv.~atomic();
        m_descritptor_cache.rtv_descriptors_lut.~unordered_map();
        break;

    case DescriptorHeapType::dsv:
        m_offset.dsv.~atomic();
        m_descritptor_cache.dsv_descriptors_lut.~unordered_map();
        break;

    default:
        LEXGINE_ASSUME;
    }
}


}