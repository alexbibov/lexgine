#ifndef LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H

#include <string>
#include <list>

#include "pipeline_state.h"
#include "../../entity.h"
#include "../../class_names.h"
#include "task_caches/hlsl_compilation_task_cache.h"
#include "../../globals.h"


namespace lexgine { namespace core { namespace dx { namespace d3d12 {

/*! Convenience class that parses supplied XML descriptions of PSO objects and constructs
 corresponding GraphicsPSODescriptor and ComputePSODescriptor structures. Note that this 
 class is NOT thread safe
*/
class D3D12PSOXMLParser : public NamedEntity<class_names::D3D12PSOXMLParser>
{
public:

    /*! Constructs the parser and immediately parses provided sources
     constructing related PSO description structures.
    */
    D3D12PSOXMLParser(core::Globals& globals, std::string const& xml_source, bool deferred_shader_compilation = true, uint32_t node_mask = 0x1);

    ~D3D12PSOXMLParser() override;

    std::vector<tasks::GraphicsPSOCompilationTask*> const& graphicsPSOCompilationTasks() const;
    std::vector<tasks::ComputePSOCompilationTask*> const& computePSOCompilationTasks() const;

private:
    class impl;

    core::Globals& m_globals;
    task_caches::HLSLCompilationTaskCache& m_hlsl_compilation_task_cache;
    task_caches::PSOCompilationTaskCache& m_pso_compilation_task_cache;

    std::vector<tasks::GraphicsPSOCompilationTask*> m_parsed_graphics_pso_compilation_tasks;
    std::vector<tasks::ComputePSOCompilationTask*> m_parsed_compute_pso_compilation_tasks;

    std::string const m_source_xml;
    uint32_t m_node_mask;
    std::unique_ptr<impl> m_impl;
};

}}}}

#define LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H
#endif