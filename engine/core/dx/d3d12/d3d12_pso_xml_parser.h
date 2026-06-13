#ifndef LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H
#define LEXGINE_CORE_DX_D3D12_D3D12_PSO_XML_PARSER_H

#include <string>
#include <list>
#include <vector>
#include <array>

#include "pipeline_state.h"
#include "engine/core/entity.h"
#include "caches/hlsl_shader_blob_cache.h"
#include "caches/lexgine_core_dx_d3d12_caches_fwd.h"
#include "caches/pso_blob_cache.h"
#include "engine/core/globals.h"


namespace lexgine::core::dx::d3d12 {

/*! Convenience class that parses supplied XML descriptions of PSO objects and constructs
 corresponding GraphicsPSODescriptor and ComputePSODescriptor structures. Note that this
 class is NOT thread safe
*/
class D3D12PSOXMLParser : public NamedEntity<D3D12PSOXMLParser>
{
public:

    /*! Constructs the parser and immediately parses provided sources
     constructing related PSO description structures.
    */
    D3D12PSOXMLParser(core::Globals& globals, std::string const& xml_source, bool deferred_shader_compilation = true, uint32_t node_mask = 0x1);

    ~D3D12PSOXMLParser() override;

    std::vector<caches::GraphicsPSOHandle> const& graphicsPSOHandles() const;
    std::vector<caches::ComputePSOHandle> const& computePSOHandles() const;

private:
    class impl;

    core::Globals& m_globals;
    caches::RootSignatureBlobCache& m_root_signature_blob_cache;
    caches::HLSLShaderBlobCache& m_hlsl_shader_blob_cache;
    caches::PSOBlobCache& m_pso_blob_cache;

    struct PendingGraphicsPSO
    {
        GraphicsPSODescriptor descriptor;
        std::array<caches::HLSLShaderHandle, 5> shader_handles;
        caches::RootSignatureHandle rs_handle;
    };

    struct PendingComputePSO
    {
        ComputePSODescriptor descriptor;
        caches::HLSLShaderHandle compute_shader_handle;
        caches::RootSignatureHandle rs_handle;
    };

    std::vector<PendingGraphicsPSO> m_pending_graphics_psos;
    std::vector<PendingComputePSO> m_pending_compute_psos;

    std::vector<caches::GraphicsPSOHandle> m_parsed_graphics_pso_handles;
    std::vector<caches::ComputePSOHandle> m_parsed_compute_pso_handles;

    // Shader tasks gathered during parsing — drained during construction to populate the PSO contracts.
    std::vector<caches::HLSLShaderHandle> m_parsed_shader_handles;

    std::string const m_source_xml;
    uint32_t m_node_mask;
    std::unique_ptr<impl> m_impl;
};

}

#endif
