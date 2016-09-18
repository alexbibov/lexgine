#include "constant_buffer.h"


using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core;


ConstantBuffer::ConstantBuffer() :
    m_current_offset{ 0 },
    m_raw_data{ nullptr },
    m_is_constructed{ false }
{
}

ConstantBuffer::ConstantBuffer(ConstantBuffer&& other) :
    m_constants{ std::move(other.m_constants) },
    m_current_offset{ other.m_current_offset },
    m_is_constructed{ other.m_is_constructed }
{
    std::swap(m_raw_data, other.m_raw_data);
}

ConstantBuffer::~ConstantBuffer()
{
    if (m_raw_data) free(m_raw_data);
}

void ConstantBuffer::build()
{
    if (!m_is_constructed)
    {
        if (m_raw_data) free(m_raw_data);
        m_raw_data = malloc(m_current_offset);

        for (auto& desc : m_constants)
        {
            void* p_constructed_data_offset = static_cast<char*>(m_raw_data) + desc.offset;
            size_t constructed_data_size = desc.p_data->size();
            memcpy(p_constructed_data_offset, desc.p_data->data(), constructed_data_size);
            desc.p_data = std::move(std::unique_ptr<DataBlob>{new DataBlob{ p_constructed_data_offset, constructed_data_size }});
        }

        m_is_constructed = true;
    }
}

size_t ConstantBuffer::size() const
{
    return m_current_offset;
}

uint32_t ConstantBuffer::add_entry(std::string const& name, DataChunk* p_data_chunk, size_t element_size)
{
    uint32_t candidate_offset = m_current_offset;
    uint32_t next_16_byte_aligned_address = (m_current_offset & 0xFFFFFFF0) + 16;
    uint32_t aligned_offset =
        candidate_offset % 16 != 0 && candidate_offset + static_cast<uint32_t>(element_size) > next_16_byte_aligned_address ?
        next_16_byte_aligned_address : candidate_offset;

    m_current_offset = aligned_offset + static_cast<uint32_t>(p_data_chunk->size());
    m_constants.emplace_back(entry_desc{ name, aligned_offset, std::unique_ptr<DataBlob>{p_data_chunk} });
    m_is_constructed = false;

    return aligned_offset;
}