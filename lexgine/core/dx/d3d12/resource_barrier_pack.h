#ifndef LEXGINE_CORE_DX_D3D12_RESOURCE_BARRIER_PACK_H
#define  LEXGINE_CORE_DX_D3D12_RESOURCE_BARRIER_PACK_H

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "resource.h"

#include <vector>

namespace lexgine::core::dx::d3d12 {

enum class SplitResourceBarrierFlags
{
    none,    //!< the resource barrier is not split
    begin,    //!< the resource barrier is "begin-only" meaning that the resource is put in no-access state
    end,    //!< the resource barrier is "complete-only", i.e. it restores access to the resource, which was previously put to no access state by a "begin-only" resource barrier
};

//! Wrapper class over multiple resource barriers combined into single object
class ResourceBarrierPack
{
public:
    ResourceBarrierPack(CommandList const& cmd_list);

    ResourceBarrierPack(ResourceBarrierPack const&) = delete;
    ResourceBarrierPack(ResourceBarrierPack&&) = default;

    ResourceBarrierPack& operator=(ResourceBarrierPack const&) = delete;
    ResourceBarrierPack& operator=(ResourceBarrierPack&&) = default;

    virtual ~ResourceBarrierPack() = default;

    void addTransitionBarrier(Resource const* p_resource, uint16_t mipmap_level, uint16_t array_layer,
        ResourceState state_before, ResourceState state_after,
        SplitResourceBarrierFlags split_barrier_flags = SplitResourceBarrierFlags::none);

    void addTransitionBarrier(Resource const* p_resource,
        ResourceState state_before, ResourceState state_after,
        SplitResourceBarrierFlags split_barrier_flags = SplitResourceBarrierFlags::none);

    void addAliasingBarrier(Resource const* p_resource_before, Resource const* p_resource_after,
        SplitResourceBarrierFlags split_barrier_flags = SplitResourceBarrierFlags::none);

    void addUAVBarrier(Resource const* p_resource,
        SplitResourceBarrierFlags split_barrier_flags = SplitResourceBarrierFlags::none);

    void applyBarriers() const;

private:
    virtual void emplaceResourceBarrier(D3D12_RESOURCE_BARRIER&& barrier) = 0;
    virtual uint32_t nativeBarrierCount() const = 0;
    virtual D3D12_RESOURCE_BARRIER const* nativeBarriers() const = 0;

private:
    CommandList const& m_command_list;
    
};


//! Resource barrier pack with capacity determined at runtime (not that it uses heap allocations, so use with care)
class DynamicResourceBarrierPack final : public ResourceBarrierPack
{
public:
    DynamicResourceBarrierPack(CommandList const& cmd_list);

private:    // required by ResourceBarrierPack interface
    void emplaceResourceBarrier(D3D12_RESOURCE_BARRIER&& barrier) override;
    uint32_t nativeBarrierCount() const override;
    D3D12_RESOURCE_BARRIER const* nativeBarriers() const override;

private:
    std::vector<D3D12_RESOURCE_BARRIER> m_barriers;    //!< list of the barriers to be applied
    
};


//! Resource barrier pack with static capacity, recommended for use within the rendering loop
template<unsigned int capacity>
class StaticResourceBarrierPack final : public ResourceBarrierPack
{
public:
    StaticResourceBarrierPack(CommandList const& cmd_list) :
        ResourceBarrierPack{ cmd_list },
        m_barrier_count{ 0U }
    {

    }

private:    // required by ResourceBarrierPack interface
    void emplaceResourceBarrier(D3D12_RESOURCE_BARRIER&& barrier) override
    {
        assert(m_barrier_count < capacity);
        m_barriers[m_barrier_count++] = std::move(barrier);
    }

    uint32_t nativeBarrierCount() const override
    {
        return m_barrier_count;
    }

    D3D12_RESOURCE_BARRIER const* nativeBarriers() const override
    {
        return m_barriers;
    }

private:
    uint32_t m_barrier_count;
    D3D12_RESOURCE_BARRIER m_barriers[capacity];
};


}

#endif    // LEXGINE_CORE_DX_D3D12_RESOURCE_BARRIER_PACK_H
