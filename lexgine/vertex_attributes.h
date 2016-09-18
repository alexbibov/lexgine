#ifndef LEXGINE_CORE_VERTEX_ATTRIBUTES_H

#include <string>
#include <list>
#include <memory>

#include <dxgi1_5.h>

#include "misc.h"
#include "d3d12_tools.h"

namespace lexgine { namespace core {


//! Abstract specification of vertex attribute
class AbstractVertexAttributeSpecification
{
private:
    template<misc::EngineAPI API> struct va_format_type;
    template<> struct va_format_type<misc::EngineAPI::Direct3D12> { using type = DXGI_FORMAT; };

public:
    enum class specification_type
    {
        per_vertex, //!< the data described by the specification is defined per-vertex
        per_instance //!< the data described by the specification is defined per-instance
    };

    std::string const& name() const { return m_semantics_name; }

    uint32_t name_index() const { return m_semantics_index; }

    virtual unsigned char id() const = 0;
    virtual unsigned char size() const = 0;
    virtual unsigned char capacity() const = 0;
    virtual specification_type type() const = 0;
    virtual unsigned int instancing_rate() const = 0;

    //! Returns format type depending on the chosen graphics API
    template<misc::EngineAPI API> typename va_format_type<API>::type format();
    template<> auto format<misc::EngineAPI::Direct3D12>() -> typename va_format_type<misc::EngineAPI::Direct3D12>::type
    {
        return d3d12VertexFormat();
    }


protected:
    AbstractVertexAttributeSpecification(char const* name, uint32_t name_index):
        m_semantics_name{ name },
        m_semantics_index{ name_index }
    {

    }

private:
    //! returns DXGI format corresponding to vertex attribute specification. This is specific to Direct3D 12 and should be used accordingly
    virtual DXGI_FORMAT d3d12VertexFormat() const = 0;

private:
    std::string m_semantics_name;    //!< string name attached to vertex attribute
    uint32_t m_semantics_index;    //!< index used when same string name is used more than once
};


//! Implements framework to define generic vertex attributes. OS- and API- agnostic
//! @param vertex_attribute_id is an index, which defines the input slot for primitive assembler
//! @param vertex_attribute_format is data type of the vertex attribute (i.e. float, int and so on). The actual choices for this parameter may depend on the underlying graphics API
//! @param vertex_attribute_size is the number of values (1-4) actually stored in the vertex attribute
//! @param normalized defines whether vertex attribute data resides in normalized format
//! @param instancing_data_rate the number of instances to draw before advancing one element in the buffer corresponding to this vertex attribute specification. Must be 0 for data specified per vertex
//! @param special_vertex_attribute_format may be used to represent some special data types not directly supported by the language (i.e. half-float or fixed). The exact use of this parameter depends on the API
template<unsigned char vertex_attribute_id, typename vertex_attribute_format, unsigned char vertex_attribute_size, bool normalized = true, unsigned int instancing_data_rate = 0,
    bool special_vertex_attribute_format = false>
class VertexAttributeSpecification final : public AbstractVertexAttributeSpecification
{
public:

    //! Initializes new vertex attribute and attaches a string name and index to it
    //! Here @param name is a string name, which gets associated with vertex attribute
    //! @param name_index is needed when same string name is associated with more than one vertex attribute.
    //! For example a 4x4 matrix with string name "MyMatrix" on shader side can be associated with 4 vertex attributes
    //! having string name "MyMatrix" name indexes from 0 to 3
    VertexAttributeSpecification(char const* name, uint32_t name_index):
        AbstractVertexAttributeSpecification{ name, name_index }
    {

    }

    unsigned char id() const override { return vertex_attribute_id; }
    unsigned char size() const override { return vertex_attribute_size; }
    unsigned char capacity() const override { return vertex_attribute_size * sizeof(value_type); }
    specification_type type() const override { return instancing_data_rate > 0 ? specification_type::per_instance : specification_type::per_vertex; }
    unsigned int instancing_rate() const override { return instancing_data_rate; }

private:
    DXGI_FORMAT d3d12VertexFormat() const override
    {
        return misc::d3d12_type_traits<vertex_attribute_format, vertex_attribute_size, special_vertex_attribute_format, normalized>::dxgi_format;
    }
};


//! OS- and API- agnostic vertex attribute specification list
using VertexAttributeSpecificationList = std::list<std::shared_ptr<AbstractVertexAttributeSpecification>>;


}}

#define LEXGINE_CORE_VERTEX_ATTRIBUTES_H
#endif