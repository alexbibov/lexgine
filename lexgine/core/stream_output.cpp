#include "stream_output.h"

using namespace lexgine::core;

StreamOutputDeclarationEntry::StreamOutputDeclarationEntry(uint32_t stream, char const * name, uint32_t name_index,
    uint8_t start_component, uint8_t component_count, uint8_t slot):
    m_stream{ stream },
    m_name{ name },
    m_name_index{ name_index },
    m_start_component{ start_component },
    m_component_count{ component_count },
    m_slot{ slot }
{
}

uint32_t StreamOutputDeclarationEntry::stream() const
{
    return m_stream;
}

std::string const& StreamOutputDeclarationEntry::name() const
{
    return m_name;
}

uint32_t StreamOutputDeclarationEntry::nameIndex() const
{
    return m_name_index;
}

std::pair<uint8_t, uint8_t> StreamOutputDeclarationEntry::outputComponents() const
{
    return std::pair<uint8_t, uint8_t>{m_start_component, m_component_count};
}

uint8_t StreamOutputDeclarationEntry::slot() const
{
    return m_slot;
}


StreamOutput::StreamOutput(std::list<lexgine::core::StreamOutputDeclarationEntry>const& so_declarations, std::list<uint32_t>const & buffer_strides, uint32_t rasterized_stream):
    so_declarations{ so_declarations }, buffer_strides{ buffer_strides }, rasterized_stream{ rasterized_stream }
{
}
