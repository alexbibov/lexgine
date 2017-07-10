#ifndef LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H

#include <string>
#include <list>

#include "pipeline_state.h"
#include "../../entity.h"
#include "../../class_names.h"
#include "task_caches/hlsl_compilation_task_cache.h"


namespace lexgine { namespace core { namespace dx { namespace d3d12 {

/*! Convenience class that parses supplied XML descriptions of PSO objects and constructs
 corresponding GraphicsPSODescriptor and ComputePSODescriptor structures. Note that this 
 class is NOT thread safe
*/
class D3D12PSOXMLParser : public NamedEntity<class_names::D3D12PSOXMLParser>
{
public:
    using const_graphics_pso_descriptor_iterator = std::list<dx::d3d12::GraphicsPSODescriptor>::const_iterator;
    using const_compute_pso_descriptor_iterator = std::list<dx::d3d12::ComputePSODescriptor>::const_iterator;

    /*! Constructs the parser and immediately parses provided sources
     constructing related PSO description structures
    */
    D3D12PSOXMLParser(std::string const& xml_source, bool deferred_shader_compilation = true);

    ~D3D12PSOXMLParser() override;

    //! returns 'true' if PSO parsing and compilation has been completed. Returns 'false' otherwise
    bool isCompleted() const;

    //! returns iterator pointing at the first parsed graphics PSO description
    const_graphics_pso_descriptor_iterator getFirstGraphicsPSOIterator() const;

    //! returns iterator pointing at the last parsed graphics PSO description
    const_graphics_pso_descriptor_iterator getLastGraphicsPSOIterator() const;


    //! returns iterator pointing at the first parsed compute PSO description
    const_compute_pso_descriptor_iterator getFirstComputePSOIterator() const;

    //! returns iterator pointing at the last parsed computed PSO description
    const_compute_pso_descriptor_iterator getLastComputePSOIterator() const;


    //! returns total amount of parsed graphics PSO descriptors
    uint32_t getNumberOfParsedGraphicsPSOs() const;

    //! returns total amount of parsed computed PSO descriptors
    uint32_t getNumberOfParsedComputePSOs() const;

private:
    struct PSOIntermediateDescriptor
    {
        std::string name;
        dx::d3d12::PSOType pso_type;
        
        union{
            dx::d3d12::GraphicsPSODescriptor graphics;
            dx::d3d12::ComputePSODescriptor compute;
        };

        PSOIntermediateDescriptor();
        ~PSOIntermediateDescriptor();

        PSOIntermediateDescriptor& operator=(PSOIntermediateDescriptor const& other);
        PSOIntermediateDescriptor& operator=(PSOIntermediateDescriptor&& other);
    };


    class impl;

    std::string const m_source_xml;
    bool m_deferred_shader_compilation;
    std::unique_ptr<impl> m_impl;
    std::list<PSOIntermediateDescriptor> m_pso_list;
    task_caches::HLSLCompilationTaskCache m_hlsl_compilation_task_cache;
};

}}}}

#define LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H
#endif