#ifndef LEXGINE_CORE_DX_D3D12_HASHABLE_DESCRIPTOR_H

#include <cstdint>
#include <cassert>
#include <algorithm>

#include "lexgine_core_dx_d3d12_fwd.h"
#include "resource.h"
#include "engine/core/misc/hash_value.h"

namespace lexgine::core::dx::d3d12 {

template<typename T>
class HashableDescriptor
{
public:
    HashableDescriptor(T const& native_ref)
        : m_native_ref{ native_ref }
    {

    }

    HashableDescriptor(Resource const& resource, T const& native_ref)
        : m_resource_ptr{ reinterpret_cast<uintptr_t>(resource.native().Get()) }
        , m_native_ref{ native_ref }
    {

    }

    misc::HashValue hash() const
    {
        misc::HashValue hash { &m_native_ref, sizeof(T) };
        hash.combine(&m_resource_ptr, sizeof(uintptr_t));
        return hash;
    }

    bool operator==(HashableDescriptor const& other) const
    {
        if (this == &other)
            return true;

        misc::HashValue thisHash = hash();
        misc::HashValue otherHash = other.hash();

        #ifdef _DEBUG
        assert((thisHash != otherHash 
            || m_resource_ptr == other.m_resource_ptr
            && std::equal(reinterpret_cast<uint8_t const*>(&m_native_ref), 
                reinterpret_cast<uint8_t const*>(&m_native_ref) + sizeof(T), 
                reinterpret_cast<uint8_t const*>(&other.m_native_ref))) && "Hash collision detected");
        #endif

        return thisHash == otherHash;
    }

private:
    uintptr_t m_resource_ptr = 0;
    T const& m_native_ref;
};

}

#define LEXGINE_CORE_DX_D3D12_HASHABLE_DESCRIPTOR_H
#endif