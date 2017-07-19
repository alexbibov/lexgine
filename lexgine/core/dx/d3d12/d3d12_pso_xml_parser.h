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
    struct GraphicsPSODescriptorCacheEntry
    {
        GraphicsPSODescriptor descriptor;
        std::string cache_name;
    };

    struct ComputePSODescriptorCacheEntry
    {
        ComputePSODescriptor descriptor;
        std::string cache_name;
    };

    using const_graphics_pso_descriptor_iterator = std::list<GraphicsPSODescriptorCacheEntry>::const_iterator;
    using const_compute_pso_descriptor_iterator = std::list<ComputePSODescriptorCacheEntry>::const_iterator;


    /*! Constructs the parser and immediately parses provided sources
     constructing related PSO description structures. 
     NOTE: parameter node_mask identifies, for which nodes to create PSO descriptors.
     For example if node_mask=0x3, all descriptors will be dubbed for the 0-th node and for the 1-st node
    */
    D3D12PSOXMLParser(core::Globals const& globals, std::string const& xml_source, bool deferred_shader_compilation = true, uint32_t node_mask = 0x1);

    ~D3D12PSOXMLParser() override;

    //! returns 'true' if PSO parsing and related shader compilation has been completed. Returns 'false' otherwise
    bool isCompleted() const;

    // blocks calling thread until compilation of all shaders is completed
    void waitUntilShadersAreCompiled() const;

    //! returns iterator pointing at the first parsed graphics PSO description
    const_graphics_pso_descriptor_iterator getFirstGraphicsPSOIterator() const;

    //! returns iterator pointing at the last parsed graphics PSO description
    const_graphics_pso_descriptor_iterator getLastGraphicsPSOIterator() const;


    //! returns iterator pointing at the first parsed compute PSO description
    const_compute_pso_descriptor_iterator getFirstComputePSOIterator() const;

    //! returns iterator pointing at the last parsed computed PSO description
    const_compute_pso_descriptor_iterator getLastComputePSOIterator() const;


    //! returns total amount of parsed graphics PSO descriptors
    size_t getNumberOfParsedGraphicsPSOs() const;

    //! returns total amount of parsed computed PSO descriptors
    size_t getNumberOfParsedComputePSOs() const;

private:
    class impl;

    core::Globals const& m_globals;
    std::string const m_source_xml;
    uint32_t m_node_mask;
    std::unique_ptr<impl> m_impl;
    std::list<GraphicsPSODescriptorCacheEntry> m_graphics_pso_descriptor_cache;
    std::list<ComputePSODescriptorCacheEntry> m_compute_pso_descriptor_cache;
    task_caches::HLSLCompilationTaskCache m_hlsl_compilation_task_cache;
};

}}}}

#define LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H
#endif