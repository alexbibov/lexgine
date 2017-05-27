#ifndef LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H

#include <string>
#include <list>

#include "pipeline_state.h"
#include "../../error_behavioral.h"

namespace lexgine { namespace core { namespace dx { namespace d3d12 {

/*! Convenience class that parses supplied XML descriptions of PSO objects and constructs
 corresponding GraphicsPSODescriptor and ComputePSODescriptor structures
*/
class D3D12PSOXMLParser : public ErrorBehavioral
{
public:
    using const_graphics_pso_descriptor_iterator = std::list<dx::d3d12::GraphicsPSODescriptor>::const_iterator;
    using const_compute_pso_descriptor_iterator = std::list<dx::d3d12::ComputePSODescriptor>::const_iterator;

    /*! Constructs the parser and immediately parses provided sources
     constructing related PSO description structures
    */
    D3D12PSOXMLParser(std::string const& xml_source);


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
    std::list<dx::d3d12::GraphicsPSODescriptor> m_graphics_pso_list;
    std::list<dx::d3d12::ComputePSODescriptor> m_compute_pso_list;
};

}}}}

#define LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H
#endif