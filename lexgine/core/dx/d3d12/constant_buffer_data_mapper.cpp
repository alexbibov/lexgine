#include "constant_buffer_data_mapper.h"

using namespace lexgine::core::dx::d3d12;

ConstantDataUpdater::ConstantDataUpdater(std::string const& variable_name, 
    std::shared_ptr<AbstractConstantDataProvider> const& constant_data_provider)
    : m_hash{ variable_name }
    , m_constant_data_provider_ptr{ constant_data_provider }
{
}

void ConstantDataUpdater::update(uint64_t constant_buffer_allocation_base_address, 
    ConstantBufferReflection const& reflection)
{
    std::shared_ptr<AbstractConstantDataProvider> constant_data_provider =
        m_constant_data_provider_ptr.lock();
    if (constant_data_provider)
    {
        uint32_t const* data = static_cast<uint32_t const*>(constant_data_provider->data());
        auto& element_reflection = reflection[m_hash];
        auto row_and_column_count = 
            ConstantBufferReflection
            ::getReflectionEntryBaseTypeDimensions(element_reflection.desc().base_type);

        uint32_t* dst_base_address =
            reinterpret_cast<uint32_t*>(constant_buffer_allocation_base_address + element_reflection.offset());
        if (element_reflection.desc().element_count > 1 || row_and_column_count.first > 1)
        {
            size_t total_elements = row_and_column_count.first*element_reflection.desc().element_count;
            
            for (int i = 0; i < total_elements; ++i)
            {
                memcpy(dst_base_address + 16 * i, data + 4 * row_and_column_count.second*i,
                    4 * row_and_column_count.second);
            }
        }
        else
        {
            memcpy(dst_base_address, data, 4 * row_and_column_count.second);
        }
    }
}
