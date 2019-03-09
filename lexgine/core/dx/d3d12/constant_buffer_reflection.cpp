#include "lexgine/core/exception.h"

#include "constant_buffer_reflection.h"


using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;


bool ConstantBufferReflection::addElement(std::string const& name, ReflectionEntryDataType const& element_type)
{
    HashedString hash{ name };
    size_t data_size;

    if (static_cast<int>(element_type.base_type) >= static_cast<int>(ReflectionEntryBaseType::float1)
        && static_cast<int>(element_type.base_type) <= static_cast<int>(ReflectionEntryBaseType::bool1))
    {
        data_size = 4 * element_type.array_elements_count;
    }
    else if (static_cast<int>(element_type.base_type) >= static_cast<int>(ReflectionEntryBaseType::float2)
        && static_cast<int>(element_type.base_type) <= static_cast<int>(ReflectionEntryBaseType::bool2x4))
    {
        m_current_offset += (8 - (m_current_offset % 8)) % 8;    // padding
        data_size = 8 * element_type.array_elements_count;
    }
    else if (static_cast<int>(element_type.base_type) >= static_cast<int>(ReflectionEntryBaseType::float3)
        && static_cast<int>(element_type.base_type) <= static_cast<int>(ReflectionEntryBaseType::bool4x4))
    {
        m_current_offset += (16 - (m_current_offset % 16)) % 16;    // padding
        data_size = 16 * element_type.array_elements_count;
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "unable to add element \"" + name + "\" into constant buffer "
            "reflection \"" + getStringName() + "\": the element has unknown data type");
    }

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
