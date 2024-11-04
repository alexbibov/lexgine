#ifndef LEXGINE_CORE_VERTEX_ATTRIBUTES_H
#define LEXGINE_CORE_VERTEX_ATTRIBUTES_H

#include <string>
#include <vector>
#include <memory>

#include <dxgi1_5.h>

#include "misc/misc.h"
#include "engine/core/engine_api.h"
#include "dx/d3d12/d3d12_tools.h"

namespace lexgine::core {


//! Abstract specification of vertex attribute
class AbstractVertexAttributeSpecification
{
private:
    template<EngineApi API> struct va_format_type;
    template<> struct va_format_type<EngineApi::Direct3D12> { using type = DXGI_FORMAT; };

public:
    enum class specification_type
    {
        per_vertex, //!< the data described by the specification is defined per-vertex
        per_instance //!< the data described by the specification is defined per-instance
    };

    virtual ~AbstractVertexAttributeSpecification() = default;

    std::string const& name() const { return m_semantics_name; }
    uint32_t name_index() const { return m_semantics_index; }
    unsigned char input_slot() const { return m_primitive_assembler_input_slot; }
    virtual unsigned char size() const = 0;
    virtual unsigned char capacity() const = 0;

    //! Returns format type depending on the chosen graphics API
    template<EngineApi API> typename va_format_type<API>::type format();
    template<> auto format<EngineApi::Direct3D12>() -> typename va_format_type<EngineApi::Direct3D12>::type
    {
        return d3d12VertexFormat();
    }

    unsigned int instancingRate() const { return m_instancing_data_rate; }
    specification_type type() const { return m_instancing_data_rate > 0 ? specification_type::per_instance : specification_type::per_vertex; }

    uint32_t offset() const { return m_element_offset; }

protected:
    AbstractVertexAttributeSpecification(unsigned char primitive_assembler_input_slot, uint32_t element_offset,
        char const* name, uint32_t name_index, uint32_t instancing_data_rate) :
        m_primitive_assembler_input_slot{ primitive_assembler_input_slot },
        m_element_offset{ element_offset },
        m_semantics_name{ name },
        m_semantics_index{ name_index },
        m_instancing_data_rate{ instancing_data_rate }
    {

    }

private:
    //! returns DXGI format corresponding to vertex attribute specification. This is specific to Direct3D 12 and should be used accordingly
    virtual DXGI_FORMAT d3d12VertexFormat() const = 0;

private:
    unsigned char m_primitive_assembler_input_slot;    //!< input slot of the primitive assembler, to which this vertex attribute is to be attached
    uint32_t m_element_offset;    //!< offset between two consequent data elements
    std::string m_semantics_name;    //!< string name attached to vertex attribute
    uint32_t m_semantics_index;    //!< index used when same string name is used more than once
    uint32_t m_instancing_data_rate;
};


/*! Implements framework to define generic vertex attributes. OS- and API- agnostic
 vertex_attribute_format is the data type of the vertex attribute (i.e. float, int and so on). The actual choices for this parameter may depend on the underlying graphics API
 vertex_attribute_size is the number of values (1-4) actually stored in the vertex attribute
 normalized defines whether vertex attribute data resides in normalized format
 */
template<typename vertex_attribute_format, unsigned char vertex_attribute_size, bool normalized = true>
class VertexAttributeSpecification final : public AbstractVertexAttributeSpecification
{
public:

    /*! Initializes new vertex attribute for requested @param primitive_assembler_input_slot and attaches a
    string @param name and integer @param name_index to it.
     Here @param name is a string name, which gets associated with vertex attribute
     parameter. Integer value @param name_index is needed when same string name is associated with more than one vertex attribute.
     One special example is a 4x4 matrix with string name "MyMatrix" on the shader side. Such matrix can be associated with up to 4 vertex attributes
     having string name "MyMatrix" and name indexes ranging from 0 to 3.
     @param element_offset determines the offset of the attribute within a vertex attribute pack.
     Finally, @param instancing_data_rate determines the number of instances to draw before advancing one element in the buffer,
     from which the data related to this vertex attribute specification are getting fetched. Must be 0 if the data are specified per vertex.
     */
    VertexAttributeSpecification(unsigned char primitive_assembler_input_slot, uint32_t element_offset,
        char const* name, uint32_t name_index, uint32_t instancing_data_rate) :
        AbstractVertexAttributeSpecification{ primitive_assembler_input_slot, element_offset, name, name_index, instancing_data_rate }
    {

    }

    unsigned char size() const override { return vertex_attribute_size; }
    unsigned char capacity() const override
    {
        return lexgine::core::dx::d3d12::d3d12_type_traits<vertex_attribute_format, vertex_attribute_size, normalized>::total_size_in_bytes;
    }

private:
    DXGI_FORMAT d3d12VertexFormat() const override
    {
        return lexgine::core::dx::d3d12::d3d12_type_traits<vertex_attribute_format, vertex_attribute_size, normalized>::dxgi_format;
    }
};


//! OS- and API- agnostic vertex attribute specification list
using VertexAttributeSpecificationList = std::vector<std::shared_ptr<AbstractVertexAttributeSpecification>>;


}

#endif