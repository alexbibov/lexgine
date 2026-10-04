#ifndef LEXGINE_CORE_DX_D3D12_BINDLESS_DESCRIPTOR_CACHE_H
#define LEXGINE_CORE_DX_D3D12_BINDLESS_DESCRIPTOR_CACHE_H

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "engine/core/entity.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "srv_descriptor.h"

namespace lexgine::core::dx::d3d12 {

//! Deduplicates shader resource views accessed through bindless descriptor tables, which span the whole descriptor heap
class BindlessDescriptorCache final : public NamedEntity<BindlessDescriptorCache>
{
public:
    explicit BindlessDescriptorCache(DescriptorAllocator& allocator);    //! descriptors are reserved using @param allocator and live until the allocator is reset

    //! returns index of the descriptor heap slot occupied by @param descriptor, creating the descriptor if it was not cached yet
    uint32_t getOrCreate(SRVDescriptor const& descriptor);

private:
    struct DescriptorHasher
    {
        size_t operator()(SRVDescriptor const& descriptor) const
        {
            return static_cast<size_t>(descriptor.hash().fold());
        }
    };

private:
    DescriptorAllocator& m_allocator;
    std::mutex m_mutex;
    std::unordered_map<SRVDescriptor, uint32_t, DescriptorHasher> m_descriptor_slots;
};

}

#endif
