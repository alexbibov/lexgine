#ifndef LEXGINE_CORE_DX_D3D12_RESOURCE_BARRIER_H

#include "resource.h"
#include "command_list.h"
#include "../../entity.h"
#include "../../class_names.h"

#include <cassert>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

enum class SplitResourceBarrierFlags
{
    none,    //!< the resource barrier is not split
    begin,    //!< the resource barrier is "begin-only" meaning that the resource is put in no-access state
    end,    //!< the resource barrier is "complete-only", i.e. it restores access to the resource, which was previously put to no access state by a "begin-only" resource barrier
};

//! Tiny object that implements accumulated resource barrier that can store up to "capacity" individual barriers
template<size_t capacity>
class ResourceBarrier final
{
    // Note, that accumulated barrier is having statically defined capacity.
    // This is needed for optimization purposes since often barriers are applied during the rendering cycle: we don't want heap allocations happening at this time!

public:
    //! creates accumulated resource barrier assigned to the given command list
    ResourceBarrier(CommandList const& cmd_list) :
        m_cmd_list{ cmd_list },
        m_num_barriers{ 0U }
    {

    }

    ResourceBarrier(ResourceBarrier const&) = delete;
    ResourceBarrier(ResourceBarrier&&) = default;

    //! adds new sub-resource state transition into the accumulated barrier. Here @param mipmap_level and @param array_layer are only valid when the corresponding sub-resources exist within the resource.
    //! The behavior is undefined if transition barrier is requested for a non-existing sub-resource
    void addTransitionBarrier(Resource const* p_resource, uint16_t mipmap_level, uint16_t array_layer, ResourceState new_state, SplitResourceBarrierFlags split_barrier_flags = SplitResourceBarrierFlags::none)
    {
        assert(m_num_barriers < capacity);

        D3D12_RESOURCE_DESC resource_desc = p_resource->native()->GetDesc();

        D3D12_RESOURCE_TRANSITION_BARRIER transition_desc;
        transition_desc.pResource = p_resource->native();
        transition_desc.Subresource = resource_desc.MipLevels*array_layer + mipmap_level;
        transition_desc.StateBefore = static_cast<D3D12_RESOURCE_STATES>(p_resource->getCurrentState().getValue());
        transition_desc.StateAfter = static_cast<D3D12_RESOURCE_STATES>(new_state.getValue());

        m_barriers[m_num_barriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        m_barriers[m_num_barriers].Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
        m_barriers[m_num_barriers].Transition = transition_desc;

        ++m_num_barriers;
    }

    //! adds new state transition of a whole resource to the accumulated barrier
    void addTransitionBarrier(Resource const* p_resource, ResourceState new_state, SplitResourceBarrierFlags split_barrier_flags /* = SplitResourceBarrierFlags::none */)
    {
        assert(m_num_barriers < capacity);

        D3D12_RESOURCE_TRANSITION_BARRIER transition_desc;
        transition_desc.pResource = p_resource->native().Get();
        transition_desc.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        transition_desc.StateBefore = static_cast<D3D12_RESOURCE_STATES>(p_resource->getCurrentState().getValue());
        transition_desc.StateAfter = static_cast<D3D12_RESOURCE_STATES>(new_state.getValue());

        m_barriers[m_num_barriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        m_barriers[m_num_barriers].Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
        m_barriers[m_num_barriers].Transition = transition_desc;

        ++m_num_barriers;
    }

    //! adds aliasing barrier into the accumulated barrier object. Note that both @param p_resource_before and @param p_resource_after can be nullptr, which
    //! indicates that any tiled resource may cause aliasing (if only one of them is nullptr, the other one is assumed to be nullptr regardless of the actual provided value)
    void addAliasingBarrier(Resource const* p_resource_before, Resource const* p_resource_after, SplitResourceBarrierFlags split_barrier_flags = SplitResourceBarrierFlags::none)
    {
        assert(m_num_barriers < capacity);

        D3D12_RESOURCE_ALIASING_BARRIER aliasing_desc;
        ComPtr<ID3D12Resource> p_before, p_after;
        if (p_resource_before == nullptr || p_resource_after == nullptr)
        {
            p_before = nullptr; p_after = nullptr;
        }
        else
        {
            p_before = p_resource_before->native();
            p_after = p_resource_after->native();
        }

        aliasing_desc.pResourceBefore = p_before.Get();
        aliasing_desc.pResourceAfter = p_after.Get();

        m_barriers[m_num_barriers].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        m_barriers[m_num_barriers].Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
        m_barriers[m_num_barriers].Aliasing = aliasing_desc;

        ++m_num_barriers;
    }

    //! adds UAV access barrier into the accumulated barrier. Note that @param p_resource may be nullptr meaning that any UAV resource may require the barrier
    void addUAVBarrier(Resource const* p_resource, SplitResourceBarrierFlags split_barrier_flags = SplitResourceBarrierFlags::none)
    {
        assert(m_num_barriers < capacity);

        D3D12_RESOURCE_UAV_BARRIER uav_desc;
        uav_desc.pResource = p_resource ? p_resource->native() : nullptr;

        m_barriers[m_num_barriers].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        m_barriers[m_num_barriers].Flags = static_cast<D3D12_RESOURCE_BARRIER_FLAGS>(split_barrier_flags);
        m_barriers[m_num_barriers].UAV = uav_desc;

        ++m_num_barriers;
    }

    //! applies resource transition barriers into the command list
    void applyBarriers() const
    {
        m_cmd_list.native()->ResourceBarrier(static_cast<UINT>(m_num_barriers), m_barriers);
    }


private:
    CommandList const& m_cmd_list;    //!< command list, to which to apply the barrier
    D3D12_RESOURCE_BARRIER m_barriers[capacity];    //!< list of the barriers to be applied
    size_t m_num_barriers;    //!< total number of barriers
};

}}}}

#define  LEXGINE_CORE_DX_D3D12_RESOURCE_BARRIER_H
#endif
