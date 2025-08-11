#include <cassert>

#include "engine/core/exception.h"
#include "constant_buffer_reflection.h"


using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;


std::pair<unsigned, unsigned> ConstantBufferReflection::getReflectionEntryBaseTypeDimensions(ReflectionEntryBaseType entry_base_type)
{
    int type_idx = static_cast<int>(entry_base_type);
    unsigned row_count = type_idx < 4 ? 1 : (type_idx - 4) / 16 + 2;
    unsigned column_count = type_idx < 4 ? 1 : (type_idx - 4) / 4 % 4 + 1;
    return std::make_pair(row_count, column_count);
}

bool ConstantBufferReflection::addElement(std::string const& name, ReflectionEntryDesc const& entry_descriptor)
{
    auto row_and_column_counts = getReflectionEntryBaseTypeDimensions(entry_descriptor.base_type);

    assert(entry_descriptor.element_count > 0);
    assert(row_and_column_counts.first > 0);
    assert(row_and_column_counts.second > 0);

    size_t next_16byte_boundary = m_current_offset + ((0x10 - (m_current_offset & 0xF)) & 0xF);
    size_t start_address, data_size;
    if (entry_descriptor.element_count > 1 || row_and_column_counts.first > 1)
    {
        start_address = next_16byte_boundary;
        data_size = 16 * (row_and_column_counts.first*entry_descriptor.element_count - 1) + 4 * row_and_column_counts.second;
    }
    else
    {
        data_size = 4 * row_and_column_counts.second;
        start_address = m_current_offset + data_size > next_16byte_boundary ? next_16byte_boundary : m_current_offset;
    }

    HashedString hash{ name };

    ReflectionEntry entry{ start_address, entry_descriptor, data_size };
    m_current_offset = start_address + data_size;

    return m_reflection_data.insert(std::make_pair(hash, entry)).second;
}

ConstantBufferReflection::ReflectionEntry const& ConstantBufferReflection::operator[](HashedString const& hash) const
{
    auto p = m_reflection_data.find(hash);
    if (p != m_reflection_data.end())
    {
        return p->second;
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, std::string{ "unable to locate entry \"" } 
            + hash.string() + "\" in constant buffer reflection \"" + getStringName() + "\"");
    }
}
