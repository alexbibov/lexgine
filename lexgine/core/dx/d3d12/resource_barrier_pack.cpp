#include "resource_barrier_pack.h"
#include "command_list.h"

using namespace lexgine::core::dx::d3d12;

void ResourceBarrierPack::addTransitionBarrier(Resource const* p_resource,
    uint16_t mipmap_level, uint16_t array_layer, 
    ResourceState state_before, ResourceState state_after, 
    SplitResourceBarrierFlags split_barrier_flags)
{
    D3D12_RESOURCE_DESC resource_desc = p_resource->native()->GetDesc();

    D3D12_RESOURCE_TRANSITION_BARRIER transition_desc;
    transition_desc.pResource = p_resource->native().Get();
    transition_desc.Subresource = resource_desc.MipLevels*array_layer + mipmap_level;
    transition_desc.StateBefore = static_cast<D3D12_RESOURCE_STATES>(state_before.getValue());
    transition_desc.StateAfter = static_cast<D3D12_RESOURCE_STATES>(state_after.getValue());

    D3D12_RESOURCE_BARRIER new_transition_barrier{};
    new_transition_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    new_transition_barrier.Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
    new_transition_barrier.Transition = transition_desc;
    emplaceResourceBarrier(std::move(new_transition_barrier));
}

void ResourceBarrierPack::addTransitionBarrier(Resource const* p_resource,
    ResourceState state_before, ResourceState state_after, 
    SplitResourceBarrierFlags split_barrier_flags)
{
    D3D12_RESOURCE_TRANSITION_BARRIER transition_desc;
    transition_desc.pResource = p_resource->native().Get();
    transition_desc.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    transition_desc.StateBefore = static_cast<D3D12_RESOURCE_STATES>(state_before.getValue());
    transition_desc.StateAfter = static_cast<D3D12_RESOURCE_STATES>(state_after.getValue());

    D3D12_RESOURCE_BARRIER new_transition_barrier{};
    new_transition_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    new_transition_barrier.Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
    new_transition_barrier.Transition = transition_desc;
    emplaceResourceBarrier(std::move(new_transition_barrier));
}

void ResourceBarrierPack::addAliasingBarrier(PlacedResource const* p_resource_before, 
    PlacedResource const* p_resource_after, SplitResourceBarrierFlags split_barrier_flags)
{
    D3D12_RESOURCE_ALIASING_BARRIER aliasing_desc;
    ComPtr<ID3D12Resource> p_before, p_after;
    p_before = p_resource_before ? p_resource_before->native() : nullptr;
    p_after = p_resource_after ? p_resource_after->native() : nullptr;

    aliasing_desc.pResourceBefore = p_before.Get();
    aliasing_desc.pResourceAfter = p_after.Get();


    D3D12_RESOURCE_BARRIER new_aliasing_barrier{};
    new_aliasing_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    new_aliasing_barrier.Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
    new_aliasing_barrier.Aliasing = aliasing_desc;
    emplaceResourceBarrier(std::move(new_aliasing_barrier));
}

void ResourceBarrierPack::addUAVBarrier(Resource const* p_resource, SplitResourceBarrierFlags split_barrier_flags)
{
    D3D12_RESOURCE_UAV_BARRIER uav_desc;
    uav_desc.pResource = p_resource ? p_resource->native().Get() : nullptr;

    D3D12_RESOURCE_BARRIER new_uav_barrier{};
    new_uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    new_uav_barrier.Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
    new_uav_barrier.UAV = uav_desc;
    emplaceResourceBarrier(std::move(new_uav_barrier));
}

void ResourceBarrierPack::applyBarriers(CommandList const& cmd_list) const
{
    cmd_list.resourceBarrier(nativeBarrierCount(), nativeBarriers());
}

void DynamicResourceBarrierPack::emplaceResourceBarrier(D3D12_RESOURCE_BARRIER&& barrier)
{
    m_barriers.emplace_back(std::move(barrier));
}

uint32_t DynamicResourceBarrierPack::nativeBarrierCount() const
{
    return static_cast<uint32_t>(m_barriers.size());
}

D3D12_RESOURCE_BARRIER const* DynamicResourceBarrierPack::nativeBarriers() const
{
    return m_barriers.data();
}
