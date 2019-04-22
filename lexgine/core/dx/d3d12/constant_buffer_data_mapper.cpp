#include <numeric>

#include "constant_buffer_data_mapper.h"

using namespace lexgine::core::dx::d3d12;

ConstantBufferDataMapper::ConstantBufferDataMapper(ConstantBufferReflection const& reflection)
    : m_reflection{ reflection }
{
}

void ConstantBufferDataMapper::writeAllBoundData(uint64_t constant_buffer_allocation_base_address) const
{
    for (auto& w : m_writers)
    {
        auto reflection_entry = m_reflection[w.first];
        
        auto row_and_column_count =
            ConstantBufferReflection
            ::getReflectionEntryBaseTypeDimensions(reflection_entry.desc().base_type);

        uint64_t dst_address = constant_buffer_allocation_base_address
            + reflection_entry.offset();

        size_t column_count = row_and_column_count.second;
        size_t element_count = w.second->dataElementCount();
        size_t total_element_count{ column_count*element_count };
        if (total_element_count > 1)
        {
            for (size_t i = 0; i < element_count; ++i)
            {
                void const* source_data = w.second->fetchDataElement(i);

                for (size_t j = 0; j < column_count; ++j)
                {
                    memcpy(reinterpret_cast<void*>(dst_address + (i*column_count + j) * 16),
                        static_cast<char const*>(source_data) + j * row_and_column_count.first * 4,
                        row_and_column_count.first * 4);
                }
            }
        }
        else
        {
            void const* source_data = w.second->fetchDataElement();
            memcpy(reinterpret_cast<void*>(dst_address), source_data, row_and_column_count.first * 4);
        }
    }
}
