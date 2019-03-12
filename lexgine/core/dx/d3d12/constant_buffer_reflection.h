#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_RELFECTION_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_REFLECTION_H

#include <map>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/misc/hashed_string.h"

namespace lexgine::core::dx::d3d12 {

//! High-level API for constant buffer usage. This API is tailored for Direct3D 12
class ConstantBufferReflection final : public NamedEntity<class_names::D3D12_ConstantBufferReflection>
{
public:
    enum class ReflectionEntryBaseType : int
    {
        float1, int1, uint1, bool1,


        float2, int2, uint2, bool2,
        float2x2, int2x2, uint2x2, bool2x2,
        float2x3, int2x3, uint2x3, bool2x3,
        float2x4, int2x4, uint2x4, bool2x4,


        float3, int3, uint3, bool3,
        float3x2, int3x2, uint3x2, bool3x2,
        float3x3, int3x3, uint3x3, bool3x3,
        float3x4, int3x4, uint3x4, bool3x4,


        float4, int4, uint4, bool4,
        float4x2, int4x2, uint4x2, bool4x2,
        float4x3, int4x3, uint4x3, bool4x3,
        float4x4, int4x4, uint4x4, bool4x4
    };

    static std::pair<unsigned, unsigned> getReflectionEntryBaseTypeDimensions(ReflectionEntryBaseType entry_base_type);

    struct ReflectionEntryDesc
    {
        ReflectionEntryBaseType base_type;
        size_t element_count;
    };

    class ReflectionEntry
    {
    public:
        ReflectionEntry(size_t offset, ReflectionEntryDesc const& entry_descriptor, size_t entry_size)
            : m_offset{ offset }
            , m_entry_descriptor{ entry_descriptor }
            , m_data_size{ entry_size }
        {}

        size_t offset() const { return m_offset; }
        ReflectionEntryDesc desc() const { return m_entry_descriptor; }
        size_t size() const { return m_data_size; }

    private:
        size_t m_offset;
        ReflectionEntryDesc m_entry_descriptor;
        size_t m_data_size;
    };


public:
    ConstantBufferReflection()
        : m_current_offset{ 0 } {}

    /*! Inserts new element into the constant buffer reflection and returns 'true' in case of success
     or 'false' if element with provided name already exists in the reflection
    */
    bool addElement(std::string const& name, ReflectionEntryDesc const& entry_descriptor);

    //! Returns the total size in bytes required to store currently assembled reflection
    inline size_t size() const { return m_current_offset; }

    //! returns reflection entry corresponding to the given name
    ReflectionEntry const& operator[](std::string const& name) const;    

    //! returns reflection entry corresponding to the given hash
    ReflectionEntry const& operator[](misc::HashedString const& hash) const;

private:
    size_t m_current_offset;
    std::map<misc::HashedString, ReflectionEntry> m_reflection_data;
};

}

#endif
