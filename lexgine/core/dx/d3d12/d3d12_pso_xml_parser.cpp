#include <algorithm>

#include "d3d12_pso_xml_parser.h"

#include "pugixml.hpp"


using namespace lexgine::core::dx::d3d12;


namespace node_names {

const char GraphicsPSO[] = "GraphicsPSO";

const char VertexShader[] = "VertexShader";
const char HullShader[] = "HullShader";
const char DomainShader[] = "DomainShader";
const char GeometryShader[] = "GeometryShader";
const char PixelShader[] = "PixelShader";



const char ComputePSO[] = "ComputePSO";

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
        if (strcmp(p_stage_name, node_names::VertexShader) == 0)
        {
            shader_type = tasks::ShaderType::vertex; 
            compilation_task_suffix = "VS";
        }
        else if (strcmp(p_stage_name, node_names::HullShader) == 0)
        {
            shader_type = tasks::ShaderType::hull;
            compilation_task_suffix = "HS";
        }
        else if (strcmp(p_stage_name, node_names::DomainShader) == 0)
        {
            shader_type = tasks::ShaderType::domain;
            compilation_task_suffix = "DS";
        }
        else if (strcmp(p_stage_name, node_names::GeometryShader) == 0)
        {
            shader_type = tasks::ShaderType::geometry;
            compilation_task_suffix = "GS";
        }
        else if (strcmp(p_stage_name, node_names::PixelShader) == 0)
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
        parseAndAddToCompilationCacheShader(node, node_names::VertexShader, true);
        parseAndAddToCompilationCacheShader(node, node_names::HullShader, false);
        parseAndAddToCompilationCacheShader(node, node_names::DomainShader, false);
        parseAndAddToCompilationCacheShader(node, node_names::GeometryShader, false);
        parseAndAddToCompilationCacheShader(node, node_names::PixelShader, true);


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
            if (std::strcmp(it->name(), node_names::GraphicsPSO) == 0)
                m_impl->parseGraphicsPSO(*it);
            else if (std::strcmp(it->name(), node_names::ComputePSO) == 0)
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
