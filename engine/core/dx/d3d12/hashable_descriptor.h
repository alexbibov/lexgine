#ifndef LEXGINE_CORE_DX_D3D12_HASHABLE_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_HASHABLE_DESCRIPTOR_H

#include <cstdint>
#include <cassert>
#include <algorithm>

#include "lexgine_core_dx_d3d12_fwd.h"
#include "resource.h"
#include "engine/core/misc/hash_value.h"
#include "engine/core/misc/hashes/xxhash128.h"

namespace lexgine::core::dx::d3d12 {

template<typename T>
class HashableDescriptor
{
public:
    HashableDescriptor()
    {
        memset(&m_native, 0, sizeof(T));
    }

    HashableDescriptor(Resource const& resource)
        : m_resource_ptr{ resource.native().Get() }
    {
        memset(&m_native, 0, sizeof(T));
    }

    misc::HashValue const& hash() const
    {
        if (!m_hash_value.isFinalized())
        {
            m_hash_value.create(&m_resource_ptr, sizeof(m_resource_ptr));
            m_hash_value.combine(&m_native, sizeof(T));
            m_hash_value.finalize();
        }
        return m_hash_value;
    }

    bool operator==(HashableDescriptor const& other) const
    {
        if (this == &other)
            return true;
        misc::HashValue const& this_hash = hash();
        misc::HashValue const& other_hash = other.hash();
        return this_hash == other_hash
            && m_resource_ptr == other.m_resource_ptr
            && memcmp(&m_native, &other.m_native, sizeof(T)) == 0;
    }

    T const& nativeDescriptor() const { return m_native; }

protected:
    T m_native;

private:
    ID3D12Resource* m_resource_ptr = nullptr;
    mutable misc::hashes::XXHash128 m_hash_value;
};

}
#endif
