#ifndef LEXGINE_CORE_DX_D3D12_UNORDERED_SRV_TABLE_ALLOCATION_MANAGER_H
#define LEXGINE_CORE_DX_D3D12_UNORDERED_SRV_TABLE_ALLOCATION_MANAGER_H

#include <atomic>
#include <unordered_map>

#include "engine/core/class_names.h"
#include "engine/core/entity.h"
#include "engine/core/dx/d3d12/hashable_descriptor.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/descriptor_heap.h"
#include "engine/core/dx/d3d12/descriptor_table_builders.h"

#include "descriptor_allocation_manager.h"


namespace lexgine::core::dx::d3d12 {

class UnorderedSRVTableAllocationManager final : public DescriptorAllocationManager
{
public:
    UnorderedSRVTableAllocationManager(DescriptorHeap& descriptor_heap);
    size_t getOrCreateDescriptor(size_t offset_hint, SRVDescriptor const& desc) override;
    

private:
    struct DescriptorHasher
    {
        size_t operator()(const SRVDescriptor& descriptor) const
        {
            return descriptor.hash().part1() ^ descriptor.hash().part2();
        }
    };

private:
    std::unordered_map<SRVDescriptor, size_t, DescriptorHasher> m_srv_lut;
};

} // namespace lexgine::core::dx::d3d12

#endif