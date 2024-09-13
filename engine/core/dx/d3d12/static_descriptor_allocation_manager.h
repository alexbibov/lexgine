#ifndef LEXGINE_CORE_DX_D3D12_STATIC_DESCRIPTOR_HEAP_ALLOCATION_MANAGER_H
#define LEXGINE_CORE_DX_D3D12_STATIC_DESCRIPTOR_HEAP_ALLOCATION_MANAGER_H

#include <atomic>
#include <unordered_map>

#include "engine/core/class_names.h"
#include "engine/core/entity.h"
#include "engine/core/dx/d3d12/hashable_descriptor.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/descriptor_table_builders.h"
#include "descriptor_allocation_manager.h"


namespace lexgine::core::dx::d3d12 {

class StaticDescriptorAllocationManager final : public DescriptorAllocationManager
                                        , public NamedEntity<class_names::D3D12_DescriptorHeapAllocationManager>
{
private:
    static constexpr float c_srv_portion = 0.7f;    // portion of the static descriptor heap capacity to be allocated for SRV descriptors
    static constexpr float c_uav_portion = 0.2f;    // portion of the static descriptor heap capacity to be allocated for UAV descriptors
    static constexpr float c_cbv_portion = 1.f - c_srv_portion - c_uav_portion;    // portion of the static descriptor heap capacity to be allocated for CBV descriptors

public:
    StaticDescriptorAllocationManager(DescriptorHeap& descriptor_heap);
    ~StaticDescriptorAllocationManager();

    uint64_t getBaseGpuAddress() const override { return m_base_gpu_address; }

    misc::Optional<UPointer> getDescriptor(CBVDescriptor const& desc) override;
    misc::Optional<UPointer> getDescriptor(SRVDescriptor const& desc) override;
    misc::Optional<UPointer> getDescriptor(UAVDescriptor const& desc) override;

    misc::Optional<UPointer> getDescriptor(SamplerDescriptor const& desc) override;

    misc::Optional<UPointer> getDescriptor(DSVDescriptor const& desc) override;
    misc::Optional<UPointer> getDescriptor(RTVDescriptor const& desc) override;


    UPointer getOrCreateDescriptor(SRVDescriptor const& desc) override;
    UPointer getOrCreateDescriptor(UAVDescriptor const& desc) override;
    UPointer getOrCreateDescriptor(CBVDescriptor const& desc) override;

    UPointer getOrCreateDescriptor(SamplerDescriptor const& desc) override;

    UPointer getOrCreateDescriptor(RTVDescriptor const& desc) override;
    UPointer getOrCreateDescriptor(DSVDescriptor const& desc) override;
    

private:
    template<typename T>
    struct DescriptorHasher
    {
        size_t operator()(const T& descriptor) const
        {
            return descriptor.hash().part1() ^ descriptor.hash().part2();
        }
    };

private:

    union {
        struct {
            uint32_t srv_descriptors_max_count;
            uint32_t uav_descriptors_max_count;
            uint32_t cbv_descriptors_max_count;
        } cbv_srv_uav;

        uint32_t sampler_descriptors_max_count;
        uint32_t dsv_descriptors_max_count;
        uint32_t rtv_descriptors_max_count;
    }m_descriptors_max_count;

    union DescriptorOffset{
        struct {
            std::atomic<uint32_t> cbv_descriptor_offset;
            std::atomic<uint32_t> srv_descriptor_offset;
            std::atomic<uint32_t> uav_descriptor_offset;
        } cbv_srv_uav;

        std::atomic<uint32_t> sampler;
        std::atomic<uint32_t> rtv;
        std::atomic<uint32_t> dsv;

        DescriptorOffset()
            : cbv_srv_uav{}
        {
            cbv_srv_uav.cbv_descriptor_offset.store(0, std::memory_order::memory_order_release);
            cbv_srv_uav.srv_descriptor_offset.store(0, std::memory_order::memory_order_release);
            cbv_srv_uav.uav_descriptor_offset.store(0, std::memory_order::memory_order_release);
        }

        ~DescriptorOffset()
        {
        }

    }m_offset;

    union DescriptorCache {
        struct {
            std::unordered_map<CBVDescriptor, UPointer, DescriptorHasher<CBVDescriptor>> cbv_descriptors_lut;
            std::unordered_map<SRVDescriptor, UPointer, DescriptorHasher<SRVDescriptor>> srv_descriptors_lut;
            std::unordered_map<UAVDescriptor, UPointer, DescriptorHasher<UAVDescriptor>> uav_descriptors_lut;
        } cbv_srv_uav;

        std::unordered_map<SamplerDescriptor, UPointer, DescriptorHasher<SamplerDescriptor>> sampler_descriptors_lut;
        std::unordered_map<RTVDescriptor, UPointer, DescriptorHasher<RTVDescriptor>> rtv_descriptors_lut;
        std::unordered_map<DSVDescriptor, UPointer, DescriptorHasher<DSVDescriptor>> dsv_descriptors_lut;

        DescriptorCache()
            : cbv_srv_uav{}
        {
         
        }

        ~DescriptorCache()
        {
        }

    }m_descritptor_cache;

    union StaticDescriptorTable
    {
        ShaderResourceDescriptorTable cbv_srv_uav;
        SamplerResourceDescriptorTable sampler;
        RenderTargetViewDescriptorTable rtv;
        DepthStencilViewDescriptorTable dsv;
    }m_descriptor_table;

    uint64_t m_base_gpu_address;

private:
    void construct(DescriptorHeapType heap_type);
    void destruct(DescriptorHeapType heap_type);
};


} // namespace lexgine::core::dx::d3d12

#endif