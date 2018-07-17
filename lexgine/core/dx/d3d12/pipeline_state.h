#ifndef LEXGINE_CORE_DX_D3D12_PIPELINE_STATE_H

#include <d3d12.h>
#include <wrl.h>

#include "../../entity.h"
#include "../../class_names.h"
#include "../../data_blob.h"
#include "../../misc/constant_converter.h"
#include "../../vertex_attributes.h"
#include "../../stream_output.h"
#include "../../multisampling.h"
#include "root_signature.h"
#include "device.h"

using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

enum class PSOType : unsigned char
{
    graphics,
    compute
};


//! Encapsulates description of graphics PSO
struct GraphicsPSODescriptor
{
    D3DDataBlob vertex_shader;    //!< vertex shader byte-code blob
    D3DDataBlob hull_shader;    //!< tessellation control (or hull in D3D terminology) shader
    D3DDataBlob domain_shader;    //!< tessellation evaluation (or domain in D3D terminology) shader
    D3DDataBlob geometry_shader;    //!< geometry shader
    D3DDataBlob pixel_shader;    //!< fragment (or pixel in D3D terminology) shader

    StreamOutput stream_output;  //!< stream output definition

    BlendState blend_state;    //!< state of the blending stage
    RasterizerDescriptor rasterization_descriptor;    //!< descriptor object encapsulating the settings affecting rasterization stage
    DepthStencilDescriptor depth_stencil_descriptor;    //!< descriptor object encapsulating the settings affecting stencil and depth stages of the pipeline
    VertexAttributeSpecificationList vertex_attributes;    //!< vertex attribute specification forwarded to the input primitive assembler
    bool primitive_restart;    //!< 'true' if primitive restart should be enabled
    PrimitiveTopology primitive_topology;    //!< primitive topology
    uint8_t num_render_targets;    //!< number of render targets (1-8 currently supported)
    DXGI_FORMAT rtv_formats[8];    //!< DXGI format for each render target in use
    DXGI_FORMAT dsv_format;    //!< DXGI depth-stencil format
    uint32_t node_mask;    //!< node mask that specifies which physical adapter in adapter link owns the PSO compiled with the given descriptor
    D3DDataBlob cached_pso;    //!< cached PSO blob

    uint32_t sample_mask;    //!< sample mask used in multi-sampling
    MultiSamplingFormat multi_sampling_format;    //!< multi-sampling format to be used by the pipeline

    //! Constructs default graphics PSO to be filled afterwards
    GraphicsPSODescriptor() = default;

    //! initializes default PSO descriptor defined as follows:
    //! 1) vertex and fragment shaders must be present. The other shader stages are not included
    //! 2) stream output is disabled
    //! 3) blending is disabled
    //! 4) rasterization is done without anti-aliasing and conservative rasterization is off
    //! 5) depth test and depth buffer updates are enabled, and primitives with smaller depth values are considered being closer to the screen
    //! 6) input vertex attributes are as defined by the caller
    //! 7) primitive restart is disabled
    //! 8) primitive topology is set to "triangle list"
    //! 9) number of render targets is 1 and its format is determined by @param rt_format
    //! 10) depth-stencil format is set to support at least 32-bit depth and at least 8-bit stencil buffers
    //! 11) node mask is as defined by @param node_mask
    GraphicsPSODescriptor(VertexAttributeSpecificationList const& vertex_attribute_specs, D3DDataBlob const& vs, D3DDataBlob const& ps, DXGI_FORMAT rt_format,
        uint32_t node_mask = 0, PrimitiveTopology topology = PrimitiveTopology::triangle);


    //! initialized PSO descriptor using precompiled PSO blob
    GraphicsPSODescriptor(D3DDataBlob const& pso_blob);
};

//! Encapsulates description of compute PSO
struct ComputePSODescriptor
{
    D3DDataBlob compute_shader;    //!< compute shader byte-core blob
    uint32_t node_mask;    //!< node mask that specifies which physical adapter in adapter link owns the PSO compiled with the given descriptor
    D3DDataBlob cached_pso;    //!< cached PSO blob

    ComputePSODescriptor() = default;    //! constructs default compute PSO to be filled afterwards
    ComputePSODescriptor(D3DDataBlob const& cs, uint32_t node_mask);
    ComputePSODescriptor(D3DDataBlob const& pso_blob);
};


//! Implements pipeline state object
class PipelineState final : public NamedEntity<class_names::D3D12PipelineState>
{
public:
    ComPtr<ID3D12PipelineState> native() const;
    Device& device() const;    //! returns device interface used to create this PSO object
    D3DDataBlob getCache() const;    //! returns cached PSO packed into data blob

    PipelineState(Device& device, D3DDataBlob const& serialized_root_signature, std::string const& root_signature_friendly_name,
        GraphicsPSODescriptor const& pso_descriptor, D3DDataBlob const& cached_pso = nullptr);    //! initializes graphics PSO

    PipelineState(Device& device, D3DDataBlob const& serialized_root_signature, std::string const& root_signature_friendly_name,
        ComputePSODescriptor const& pso_descriptor, D3DDataBlob const& cached_pso = nullptr);    //! initializes compute PSO

    PipelineState(PipelineState const&) = delete;
    PipelineState(PipelineState&&) = default;


    void setStringName(std::string const& entity_string_name);	//! sets new user-friendly string name for the pipeline state object

private:
    Device& m_device;    //!< device interface that was used to create this PSO
    ComPtr<ID3D12PipelineState> m_pipeline_state;    //! pointer to the native ID3D12PipelineState interface encapsulated by the wrapper
};

}}}}

#define LEXGINE_CORE_DX_D3D12_PIPELINE_STATE_H
#endif
