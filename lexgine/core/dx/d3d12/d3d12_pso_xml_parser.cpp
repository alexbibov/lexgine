#include <algorithm>

#include "d3d12_pso_xml_parser.h"

#include "pugixml.hpp"


using namespace lexgine::core::dx::d3d12;


namespace {

enum class attribute_type {
    boolean,
    primitive_topology,
    unsigned_numeric,
    blend_factor,
    blend_operation,
    logical_operation,
    fill_mode,
    face,
    winding_mode,
    comparison_function,
    stencil_operation,
    depth_stencil_format,
    render_target_format
};

bool isNullAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "NULL") == 0;
}

bool isBooleanAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "TRUE") == 0
        || std::strcmp(attribute.value(), "FALSE") == 0;
}

bool GetBooleanFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "TRUE") == 0) return true;
    if (std::strcmp(attribute.value(), "FALSE") == 0) return false;

    return false;
}

bool isPrimitiveTopologyAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "POINT") == 0
        || std::strcmp(attribute.value(), "LINE") == 0
        || std::strcmp(attribute.value(), "TRIANGLE") == 0
        || std::strcmp(attribute.value(), "PATCH") == 0;
}

lexgine::core::PrimitiveTopology getPrimitiveTopologyFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "POINT") == 0) return lexgine::core::PrimitiveTopology::point;
    if (std::strcmp(attribute.value(), "LINE") == 0) return lexgine::core::PrimitiveTopology::line;
    if (std::strcmp(attribute.value(), "TRIANGLE") == 0) return lexgine::core::PrimitiveTopology::triangle;
    if (std::strcmp(attribute.value(), "PATCH") == 0) return lexgine::core::PrimitiveTopology::patch;

    return lexgine::core::PrimitiveTopology::triangle;
}

bool isUnsignedNumericAttribute(pugi::xml_attribute& attribute)
{
    return attribute.as_ullong(0xFFFFFFFFFFFFFFFF) < 0xFFFFFFFFFFFFFFFF;
}

uint32_t getUnsignedNumericFromAttribute(pugi::xml_attribute& attribute)
{
    return attribute.as_uint();
}

bool isBlendFactorAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "ZERO") == 0
        || std::strcmp(attribute.value(), "ONE") == 0
        || std::strcmp(attribute.value(), "SOURCE_COLOR") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_COLOR") == 0
        || std::strcmp(attribute.value(), "SOURCE_ALPHA") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_ALPHA") == 0
        || std::strcmp(attribute.value(), "DESTINATION_ALPHA") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_ALPHA") == 0
        || std::strcmp(attribute.value(), "DESTINATION_COLOR") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_COLOR") == 0
        || std::strcmp(attribute.value(), "SOURCE_ALPHA_SATURATION") == 0
        || std::strcmp(attribute.value(), "CUSTOM_CONSTANT") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_CUSTOM_CONSTANT") == 0
        || std::strcmp(attribute.value(), "SOURCE1_COLOR") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_COLOR") == 0
        || std::strcmp(attribute.value(), "SOURCE1_ALPHA") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_ALPHA") == 0;
}

lexgine::core::BlendFactor getBlendFactorFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "ZERO") == 0) return lexgine::core::BlendFactor::zero;
    if (std::strcmp(attribute.value(), "ONE") == 0) return lexgine::core::BlendFactor::one;
    if (std::strcmp(attribute.value(), "SOURCE_COLOR") == 0) return lexgine::core::BlendFactor::source_color;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_COLOR") == 0) return lexgine::core::BlendFactor::one_minus_source_color;
    if (std::strcmp(attribute.value(), "SOURCE_ALPHA") == 0) return lexgine::core::BlendFactor::source_alpha;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_ALPHA") == 0) return lexgine::core::BlendFactor::one_minus_source_alpha;
    if (std::strcmp(attribute.value(), "DESTINATION_ALPHA") == 0) return lexgine::core::BlendFactor::destination_alpha;
    if (std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_ALPHA") == 0) return lexgine::core::BlendFactor::one_minus_destination_alpha;
    if (std::strcmp(attribute.value(), "DESTINATION_COLOR") == 0) return lexgine::core::BlendFactor::destination_color;
    if (std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_COLOR") == 0) return lexgine::core::BlendFactor::one_minus_destination_color;
    if (std::strcmp(attribute.value(), "SOURCE_ALPHA_SATURATION") == 0) return lexgine::core::BlendFactor::source_alpha_saturation;
    if (std::strcmp(attribute.value(), "CUSTOM_CONSTANT") == 0)return lexgine::core::BlendFactor::custom_constant;
    if (std::strcmp(attribute.value(), "ONE_MINUS_CUSTOM_CONSTANT") == 0) return lexgine::core::BlendFactor::one_minus_custom_constant;
    if (std::strcmp(attribute.value(), "SOURCE1_COLOR") == 0)return lexgine::core::BlendFactor::source1_color;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_COLOR") == 0) return lexgine::core::BlendFactor::one_minus_source1_color;
    if (std::strcmp(attribute.value(), "SOURCE1_ALPHA") == 0)return lexgine::core::BlendFactor::source1_alpha;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_ALPHA") == 0) return lexgine::core::BlendFactor::one_minus_source1_alpha;

    return lexgine::core::BlendFactor::zero;
}

bool isBlendOperationAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "ADD") == 0
        || std::strcmp(attribute.value(), "SUBTRACT") == 0
        || std::strcmp(attribute.value(), "REVERSE_SUBTRACT") == 0
        || std::strcmp(attribute.value(), "MIN") == 0
        || std::strcmp(attribute.value(), "MAX") == 0;
}

lexgine::core::BlendOperation getBlendOperationFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "ADD") == 0) return lexgine::core::BlendOperation::add;
    if (std::strcmp(attribute.value(), "SUBTRACT") == 0) return lexgine::core::BlendOperation::subtract;
    if (std::strcmp(attribute.value(), "REVERSE_SUBTRACT") == 0) return lexgine::core::BlendOperation::reverse_subtract;
    if (std::strcmp(attribute.value(), "MIN") == 0) return lexgine::core::BlendOperation::min;
    if (std::strcmp(attribute.value(), "MAX") == 0) return lexgine::core::BlendOperation::max;

    return lexgine::core::BlendOperation::add;
}

}


class lexgine::core::dx::d3d12::D3D12PSOXMLParser::impl
{
public:
    impl(D3D12PSOXMLParser& parent) :
        m_parent{ parent }
    {

    }

    void parseAndAddToCompilationCacheShader(pugi::xml_node& node, char const* p_stage_name, bool is_obligatory_shader_stage)
    {
        // parse and attempt to compile vertex shader
        auto shader_node = node.find_child([p_stage_name](pugi::xml_node& n) -> bool
        {
            return strcmp(n.name(), p_stage_name) == 0;
        });

        if (is_obligatory_shader_stage && shader_node.empty())
        {
            m_parent.raiseError(R"*(Error while parsing D3D12 PSO XML description from source ")*"
                + m_parent.m_source_xml + R"*(": obligatory attribute )*" + p_stage_name + " is not found");
            return;
        }

        pugi::char_t const* shader_source_location = shader_node.child_value();

        tasks::ShaderType shader_type;
        char* compilation_task_suffix = "";
        if (strcmp(p_stage_name, "VertexShader") == 0)
        {
            shader_type = tasks::ShaderType::vertex;
            compilation_task_suffix = "VS";
        }
        else if (strcmp(p_stage_name, "HullShader") == 0)
        {
            shader_type = tasks::ShaderType::hull;
            compilation_task_suffix = "HS";
        }
        else if (strcmp(p_stage_name, "DomainShader") == 0)
        {
            shader_type = tasks::ShaderType::domain;
            compilation_task_suffix = "DS";
        }
        else if (strcmp(p_stage_name, "GeometryShader") == 0)
        {
            shader_type = tasks::ShaderType::geometry;
            compilation_task_suffix = "GS";
        }
        else if (strcmp(p_stage_name, "PixelShader") == 0)
        {
            shader_type = tasks::ShaderType::pixel;
            compilation_task_suffix = "PS";
        }

        pugi::char_t const* shader_entry_point_name = shader_node.attribute("entry").as_string();

        m_parent.m_hlsl_compilation_task_cache.addTask(shader_source_location, m_currently_assembled_pso_descriptor.name + "_"
            + shader_source_location + "_" + compilation_task_suffix, shader_type, shader_entry_point_name,
            lexgine::core::ShaderSourceCodePreprocessor::SourceType::file);
    }

    void parseGraphicsPSO(pugi::xml_node& node)
    {
        // Get attributes of the graphics PSO
        m_currently_assembled_pso_descriptor.name = node.attribute("name").as_string(("GraphicsPSO_" + m_parent.getId().toString()).c_str());
        m_currently_assembled_pso_descriptor.pso_type = dx::d3d12::PSOType::graphics;



        // Retrieve shader stages
        parseAndAddToCompilationCacheShader(node, "VertexShader", true);
        parseAndAddToCompilationCacheShader(node, "HullShader", false);
        parseAndAddToCompilationCacheShader(node, "DomainShader", false);
        parseAndAddToCompilationCacheShader(node, "GeometryShader", false);
        parseAndAddToCompilationCacheShader(node, "PixelShader", true);


    }

    void parseComputePSO(pugi::xml_node& node)
    {

    }

private:
    D3D12PSOXMLParser& m_parent;
    PSOIntermediateDescriptor m_currently_assembled_pso_descriptor;
};


lexgine::core::dx::d3d12::D3D12PSOXMLParser::D3D12PSOXMLParser(std::string const& xml_source, bool deferred_shader_compilation) :
    m_source_xml{ xml_source },
    m_deferred_shader_compilation{ deferred_shader_compilation },
    m_impl{ new impl{*this} }
{
    pugi::xml_document xml_doc;
    pugi::xml_parse_result parse_result = xml_doc.load_string(xml_source.c_str());

    if (parse_result)
    {
        for (pugi::xml_node_iterator it : xml_doc)
        {
            if (std::strcmp(it->name(), "GraphicsPSO") == 0)
                m_impl->parseGraphicsPSO(*it);
            else if (std::strcmp(it->name(), "ComputePSO") == 0)
                m_impl->parseComputePSO(*it);
            else
            {
                ErrorBehavioral::raiseError(std::string{ R"*(Unknown attribute ")*" }
                +it->name() + R"*(" while parsing D3D12 PSO XML description from file ")*" + xml_source + R"*(")*");
                return;
            }
        }
    }
    else
    {
        ErrorBehavioral::raiseError(
            std::string{ "Unable to parse XML source\n"
            "Reason: " } + parse_result.description() + "\n"
            "Location: " + std::string{ xml_source.c_str() + parse_result.offset, std::min<size_t>(xml_source.size() - parse_result.offset, 80) - 1 }
        );
    }
}

lexgine::core::dx::d3d12::D3D12PSOXMLParser::~D3D12PSOXMLParser() = default;

lexgine::core::dx::d3d12::D3D12PSOXMLParser::PSOIntermediateDescriptor::PSOIntermediateDescriptor() :
    pso_type{ dx::d3d12::PSOType::graphics },
    graphics{}
{

}

D3D12PSOXMLParser::PSOIntermediateDescriptor::~PSOIntermediateDescriptor()
{
    switch (pso_type)
    {
    case dx::d3d12::PSOType::graphics:
        graphics.~GraphicsPSODescriptor();
        break;
    case d3d12::PSOType::compute:
        compute.~ComputePSODescriptor();
        break;
    }
}

D3D12PSOXMLParser::PSOIntermediateDescriptor& D3D12PSOXMLParser::PSOIntermediateDescriptor::operator=(PSOIntermediateDescriptor const& other)
{
    if (this == &other)
        return *this;

    name = other.name;
    pso_type = other.pso_type;

    if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        graphics = other.graphics;
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        compute = other.compute;
    }
    else if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        graphics.~GraphicsPSODescriptor();
        compute = other.compute;
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        compute.~ComputePSODescriptor();
        graphics = other.graphics;
    }

    return *this;
}

D3D12PSOXMLParser::PSOIntermediateDescriptor& D3D12PSOXMLParser::PSOIntermediateDescriptor::operator=(PSOIntermediateDescriptor&& other)
{
    if (this == &other)
        return *this;

    name = std::move(other.name);
    pso_type = std::move(other.pso_type);

    if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        graphics = std::move(other.graphics);
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        compute = std::move(other.compute);
    }
    else if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        graphics.~GraphicsPSODescriptor();
        compute = std::move(other.compute);
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        compute.~ComputePSODescriptor();
        graphics = std::move(other.graphics);
    }

    return *this;
}
