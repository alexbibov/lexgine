#include "lexgine/core/exception.h"

#include "constant_buffer_reflection.h"


using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;


bool ConstantBufferReflection::addElement(std::string const& name, ReflectionEntryDataType const& element_type)
{
    HashedString hash{ name };
    size_t element_size;

    size_t next_16byte_boundary = (m_current_offset & (~0xFui64)) + 0x10ui64;

    if (static_cast<int>(element_type.base_type) >= static_cast<int>(ReflectionEntryBaseType::float1)
        && static_cast<int>(element_type.base_type) <= static_cast<int>(ReflectionEntryBaseType::bool1))
    {
        element_size = element_type.array_elements_count > 1 ? 16 : 4;
    }
    else if (static_cast<int>(element_type.base_type) >= static_cast<int>(ReflectionEntryBaseType::float2)
        && static_cast<int>(element_type.base_type) <= static_cast<int>(ReflectionEntryBaseType::bool2x4))
    {
        element_size = element_type.array_elements_count > 1 ? 16 : 4;
    }
    else if (static_cast<int>(element_type.base_type) >= static_cast<int>(ReflectionEntryBaseType::float3)
        && static_cast<int>(element_type.base_type) <= static_cast<int>(ReflectionEntryBaseType::bool4x4))
    {
        
        element_size = element_type.array_elements_count > 1 ? 16 : 4;
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "unable to add element \"" + name + "\" into constant buffer "
            "reflection \"" + getStringName() + "\": the element has unknown data type");
    }

    if (m_current_offset + element_size > next_16byte_boundary) m_current_offset = next_16byte_boundary;
    size_t data_size = element_size * element_type.array_elements_count;

    ReflectionEntry entry{ m_current_offset, element_type, data_size };
    m_current_offset += data_size;

    return m_reflection_data.insert(std::make_pair(hash, entry)).second;
}

ConstantBufferReflection::ReflectionEntry const& ConstantBufferReflection::operator[](std::string const& name) const
{
    HashedString hash{ name };
    auto p = m_reflection_data.find(hash);
    if (p != m_reflection_data.end())
    {
        return p->second;
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "unable to locate entry \"" + name + "\" in constant "
            "buffer reflection \"" + getStringName() + "\"");
    }
}
