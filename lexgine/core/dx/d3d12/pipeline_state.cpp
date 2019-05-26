#include "lexgine/core/exception.h"
#include "lexgine/core/globals.h"

#include "pipeline_state.h"
#include "d3d12_tools.h"
#include "device.h"
#include "dx_resource_factory.h"


using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;

namespace {

bool isWARPAdapterCurrentlySelected(Globals const& globals)
{
    DxResourceFactory const& dx_resource_factory_ref = *globals.get<DxResourceFactory>();
    Device const& current_device_ref = *globals.get<Device>();
    Device const& warp_device_ref = dx_resource_factory_ref.hardwareAdapterEnumerator().getWARPAdapter().device();

    return &current_device_ref == &warp_device_ref;
}

bool isDebugModeEnabled(Globals const& globals)
{
    DxResourceFactory const& dx_resource_factory_ref = *globals.get<DxResourceFactory>();
    return dx_resource_factory_ref.debugInterface() != nullptr;
}

}

ComPtr<ID3D12PipelineState> PipelineState::native() const
{
    return m_pipeline_state;
}

Device& PipelineState::device() const
{
    return m_device;
}

D3DDataBlob PipelineState::getCache() const
{
    ID3DBlob* p_blob;
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_pipeline_state->GetCachedBlob(&p_blob),
        S_OK
    );
    return D3DDataBlob{ p_blob };
}

PipelineState::PipelineState(Globals& globals, D3DDataBlob const& serialized_root_signature, std::string const& root_signature_friendly_name,
    GraphicsPSODescriptor const& pso_descriptor, D3DDataBlob const& cached_pso):
    m_device{ *globals.get<Device>() }
{
    ComPtr<ID3D12RootSignature> root_signature = m_device.createRootSignature(serialized_root_signature, root_signature_friendly_name, pso_descriptor.node_mask);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
    memset(&desc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    // Define basic parts of the PSO
    desc.pRootSignature = root_signature.Get();
    desc.VS = D3D12_SHADER_BYTECODE{ pso_descriptor.vertex_shader.data(), pso_descriptor.vertex_shader.size() };
    desc.PS = D3D12_SHADER_BYTECODE{ pso_descriptor.pixel_shader.data(), pso_descriptor.pixel_shader.size() };
    desc.DS = D3D12_SHADER_BYTECODE{ pso_descriptor.domain_shader.data(), pso_descriptor.domain_shader.size() };
    desc.HS = D3D12_SHADER_BYTECODE{ pso_descriptor.hull_shader.data(), pso_descriptor.hull_shader.size() };
    desc.GS = D3D12_SHADER_BYTECODE{ pso_descriptor.geometry_shader.data(), pso_descriptor.geometry_shader.size() };


    // Define stream output
    D3D12_STREAM_OUTPUT_DESC so_desc;
    so_desc.NumEntries = static_cast<UINT>(pso_descriptor.stream_output.so_declarations.size());

    //D3D12_SO_DECLARATION_ENTRY *p_so_declaration_entries = new D3D12_SO_DECLARATION_ENTRY[so_desc.NumEntries];
    std::vector<D3D12_SO_DECLARATION_ENTRY> so_declaration_entries(so_desc.NumEntries);

    uint32_t so_declaration_entry_idx = 0;
    for (auto p = pso_descriptor.stream_output.so_declarations.begin(); p != pso_descriptor.stream_output.so_declarations.end(); ++p, ++so_declaration_entry_idx)
    {
        std::pair<LPCSTR, UINT> semantic_name_and_index = p->name() == "NULL" ?
            std::make_pair<LPCSTR, UINT>(NULL, 0U)
            : std::make_pair<LPCSTR, UINT>(p->name().c_str(), p->nameIndex());

        so_declaration_entries[so_declaration_entry_idx].Stream = p->stream();
        so_declaration_entries[so_declaration_entry_idx].SemanticName = semantic_name_and_index.first;
        so_declaration_entries[so_declaration_entry_idx].SemanticIndex = semantic_name_and_index.second;
        auto element_components = p->outputComponents();
        so_declaration_entries[so_declaration_entry_idx].StartComponent = element_components.first;
        so_declaration_entries[so_declaration_entry_idx].ComponentCount = element_components.second;
        so_declaration_entries[so_declaration_entry_idx].OutputSlot = p->slot();
    }
    so_desc.pSODeclaration = so_declaration_entries.size() ? so_declaration_entries.data() : NULL;

    so_desc.NumStrides = static_cast<UINT>(pso_descriptor.stream_output.buffer_strides.size());

    //UINT *p_buffer_strides = new UINT[so_desc.NumStrides];
    std::vector<UINT> buffer_strides(so_desc.NumStrides);
    uint32_t buffer_stride_idx = 0;
    for (auto p = pso_descriptor.stream_output.buffer_strides.begin(); p != pso_descriptor.stream_output.buffer_strides.end(); ++p, ++buffer_stride_idx)
        buffer_strides[buffer_stride_idx] = *p;
    so_desc.pBufferStrides = buffer_strides.size() ? buffer_strides.data() : NULL;

    so_desc.RasterizedStream = pso_descriptor.stream_output.rasterized_stream;

    desc.StreamOutput = so_desc;


    // Define blend state
    {
        D3D12_BLEND_DESC blend_desc;
        memset(&blend_desc, 0, sizeof(D3D12_BLEND_DESC));

        blend_desc.AlphaToCoverageEnable = pso_descriptor.blend_state.alphaToCoverageEnable;
        blend_desc.IndependentBlendEnable = pso_descriptor.blend_state.independentBlendEnable;
        for (uint8_t i = 0U; i < pso_descriptor.num_render_targets; ++i)
        {
            blend_desc.RenderTarget[i].BlendEnable = pso_descriptor.blend_state.render_target_blend_descriptor[i].isEnabled();
            blend_desc.RenderTarget[i].LogicOpEnable = pso_descriptor.blend_state.render_target_blend_descriptor[i].isLogicalOperationEnabled();
            auto src_blend_factors = pso_descriptor.blend_state.render_target_blend_descriptor[i].getSourceBlendFactors();
            auto dst_blend_factors = pso_descriptor.blend_state.render_target_blend_descriptor[i].getDestinationBlendFactors();
            auto blend_op = pso_descriptor.blend_state.render_target_blend_descriptor[i].getBlendOperation();
            blend_desc.RenderTarget[i].SrcBlend = d3d12Convert(src_blend_factors.first);
            blend_desc.RenderTarget[i].DestBlend = d3d12Convert(dst_blend_factors.first);
            blend_desc.RenderTarget[i].BlendOp = d3d12Convert(blend_op.first);
            blend_desc.RenderTarget[i].SrcBlendAlpha = d3d12Convert(src_blend_factors.second);
            blend_desc.RenderTarget[i].DestBlendAlpha = d3d12Convert(dst_blend_factors.second);
            blend_desc.RenderTarget[i].BlendOpAlpha = d3d12Convert(blend_op.second);
            blend_desc.RenderTarget[i].LogicOp = d3d12Convert(pso_descriptor.blend_state.render_target_blend_descriptor[i].getBlendLogicalOperation());
            blend_desc.RenderTarget[i].RenderTargetWriteMask = pso_descriptor.blend_state.render_target_blend_descriptor[i].getColorWriteMask().getValue();
        }
        desc.BlendState = blend_desc;
    }

    desc.SampleMask = pso_descriptor.sample_mask;


    // Define rasterizer state
    {
        D3D12_RASTERIZER_DESC rasterizer_desc;
        rasterizer_desc.FillMode = d3d12Convert(pso_descriptor.rasterization_descriptor.getFillMode());
        rasterizer_desc.CullMode = d3d12Convert(pso_descriptor.rasterization_descriptor.getCullMode());
        rasterizer_desc.FrontCounterClockwise = pso_descriptor.rasterization_descriptor.getWindingOrder() == FrontFaceWinding::counterclockwise;
        rasterizer_desc.DepthBias = pso_descriptor.rasterization_descriptor.getDepthBias();
        rasterizer_desc.DepthBiasClamp = pso_descriptor.rasterization_descriptor.getDepthBiasClamp();
        rasterizer_desc.SlopeScaledDepthBias = pso_descriptor.rasterization_descriptor.getSlopeScaledDepthBias();
        rasterizer_desc.DepthClipEnable = pso_descriptor.rasterization_descriptor.isDepthClipEnabled();
        rasterizer_desc.MultisampleEnable = pso_descriptor.rasterization_descriptor.isMultisamplingEnabled();
        rasterizer_desc.AntialiasedLineEnable = pso_descriptor.rasterization_descriptor.isAntialiasedLineDrawingEnabled();
        rasterizer_desc.ForcedSampleCount = 0;    // sample count is never forced while sampling from UAVs
        rasterizer_desc.ConservativeRaster = d3d12Convert(pso_descriptor.rasterization_descriptor.getConservativeRasterizationMode());

        desc.RasterizerState = rasterizer_desc;
    }


    // Define depth-stencil state
    {
        D3D12_DEPTH_STENCIL_DESC depth_stencil_desc;
        depth_stencil_desc.DepthEnable = pso_descriptor.depth_stencil_descriptor.isDepthTestEnabled();
        depth_stencil_desc.DepthWriteMask = pso_descriptor.depth_stencil_descriptor.isDepthUpdateAllowed() ? D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
        depth_stencil_desc.DepthFunc = d3d12Convert(pso_descriptor.depth_stencil_descriptor.depthTestPredicate());
        depth_stencil_desc.StencilEnable = pso_descriptor.depth_stencil_descriptor.isStencilTestEnabled();
        depth_stencil_desc.StencilReadMask = pso_descriptor.depth_stencil_descriptor.stencilReadMask();
        depth_stencil_desc.StencilWriteMask = pso_descriptor.depth_stencil_descriptor.stencilWriteMask();
        StencilBehavior ff_sb = pso_descriptor.depth_stencil_descriptor.stencilBehaviorForFrontFacingTriangles();
        StencilBehavior bf_sb = pso_descriptor.depth_stencil_descriptor.stencilBehaviorForBackFacingTriangles();
        depth_stencil_desc.FrontFace.StencilFailOp = d3d12Convert(ff_sb.st_fail);
        depth_stencil_desc.FrontFace.StencilDepthFailOp = d3d12Convert(ff_sb.st_pass_dt_fail);
        depth_stencil_desc.FrontFace.StencilPassOp = d3d12Convert(ff_sb.st_pass_dt_pass);
        depth_stencil_desc.FrontFace.StencilFunc = d3d12Convert(ff_sb.cmp_fun);
        depth_stencil_desc.BackFace.StencilFailOp = d3d12Convert(bf_sb.st_fail);
        depth_stencil_desc.BackFace.StencilDepthFailOp = d3d12Convert(bf_sb.st_pass_dt_fail);
        depth_stencil_desc.BackFace.StencilPassOp = d3d12Convert(bf_sb.st_pass_dt_pass);
        depth_stencil_desc.BackFace.StencilFunc = d3d12Convert(bf_sb.cmp_fun);

        desc.DepthStencilState = depth_stencil_desc;
    }


    // Vertex attributes input layout specification
    D3D12_INPUT_LAYOUT_DESC input_layout_desc;
    input_layout_desc.NumElements = static_cast<UINT>(pso_descriptor.vertex_attributes.size());
    //D3D12_INPUT_ELEMENT_DESC* p_input_element_descs = new D3D12_INPUT_ELEMENT_DESC[input_layout_desc.NumElements];
    std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_descs(input_layout_desc.NumElements);
    input_layout_desc.pInputElementDescs = input_element_descs.data();

    size_t input_element_desc_idx = 0;
    for (auto p = pso_descriptor.vertex_attributes.begin(); p != pso_descriptor.vertex_attributes.end(); ++p, ++input_element_desc_idx)
    {
        input_element_descs[input_element_desc_idx].SemanticName = (**p).name().c_str();
        input_element_descs[input_element_desc_idx].SemanticIndex = (**p).name_index();
        input_element_descs[input_element_desc_idx].Format = (**p).format<EngineAPI::Direct3D12>();
        input_element_descs[input_element_desc_idx].InputSlot = (**p).input_slot();
        input_element_descs[input_element_desc_idx].AlignedByteOffset = (**p).offset();
        input_element_descs[input_element_desc_idx].InputSlotClass = (**p).type() == AbstractVertexAttributeSpecification::specification_type::per_instance ?
            D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_element_descs[input_element_desc_idx].InstanceDataStepRate = (**p).instancingRate();
    }

    desc.InputLayout = input_layout_desc;


    // Primitive restart configuration
    desc.IBStripCutValue = pso_descriptor.primitive_restart ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE::D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE::D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;


    // Primitive topology configuration
    desc.PrimitiveTopologyType = d3d12Convert(pso_descriptor.primitive_topology_type);


    // Miscellaneous
    desc.NumRenderTargets = pso_descriptor.num_render_targets;
    for (uint8_t i = 0; i < pso_descriptor.num_render_targets; ++i) desc.RTVFormats[i] = pso_descriptor.rtv_formats[i];
    desc.DSVFormat = pso_descriptor.dsv_format;
    desc.SampleDesc.Count = pso_descriptor.multi_sampling_format.count;
    desc.SampleDesc.Quality = pso_descriptor.multi_sampling_format.quality;
    desc.NodeMask = pso_descriptor.node_mask;

    if (cached_pso)
        desc.CachedPSO = D3D12_CACHED_PIPELINE_STATE{ cached_pso.data(), cached_pso.size() };
    else
        desc.CachedPSO = D3D12_CACHED_PIPELINE_STATE{ NULL, 0U };

    desc.Flags = isWARPAdapterCurrentlySelected(globals) && isDebugModeEnabled(globals) ? 
        D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG
        : D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

    LEXGINE_THROW_ERROR_IF_FAILED(
        this, 
        m_device.native()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state)), 
        S_OK
    );

    // delete[] p_so_declaration_entries;
    // delete[] p_buffer_strides;
    // delete[] p_input_element_descs;
}

PipelineState::PipelineState(Globals& globals, D3DDataBlob const& serialized_root_signature, std::string const& root_signature_friendly_name,
    ComputePSODescriptor const & pso_descriptor, D3DDataBlob const& cached_pso):
    m_device{ *globals.get<Device>() }
{
    ComPtr<ID3D12RootSignature> root_signature = m_device.createRootSignature(serialized_root_signature, root_signature_friendly_name, pso_descriptor.node_mask);

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
    desc.pRootSignature = root_signature.Get();
    desc.CS = D3D12_SHADER_BYTECODE{ pso_descriptor.compute_shader.data(), pso_descriptor.compute_shader.size() };
    desc.NodeMask = pso_descriptor.node_mask;

    if (cached_pso)
        desc.CachedPSO = D3D12_CACHED_PIPELINE_STATE{ cached_pso.data(), cached_pso.size() };
    else
        desc.CachedPSO = D3D12_CACHED_PIPELINE_STATE{ NULL, 0U };

    desc.Flags = isWARPAdapterCurrentlySelected(globals) && isDebugModeEnabled(globals) ?
        D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG
        : D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;

    LEXGINE_THROW_ERROR_IF_FAILED(
        this, 
        m_device.native()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state)), 
        S_OK
    );
}

void PipelineState::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);
    m_pipeline_state->SetName(misc::asciiStringToWstring(entity_string_name).c_str());
}

GraphicsPSODescriptor::GraphicsPSODescriptor()
    : primitive_restart{ true }
    , node_mask{ 0x1 }
    , sample_mask{ 0xFFFFFFFF }
{
}

GraphicsPSODescriptor::GraphicsPSODescriptor(VertexAttributeSpecificationList const& vertex_attribute_specs, D3DDataBlob const& vs, D3DDataBlob const& ps,
    DXGI_FORMAT rt_format, uint32_t node_mask, PrimitiveTopologyType topology) :
    vertex_shader{ vs },
    pixel_shader{ ps },
    vertex_attributes{ vertex_attribute_specs },
    primitive_restart{ false },
    primitive_topology_type{ topology },
    num_render_targets{ 1 },
    rtv_formats{ rt_format },
    dsv_format{ DXGI_FORMAT_D32_FLOAT_S8X24_UINT },
    node_mask{ node_mask },
    sample_mask{ 0xFFFFFFFF },
    multi_sampling_format{ 1, 0 }
{

}

GraphicsPSODescriptor::GraphicsPSODescriptor(D3DDataBlob const & pso_blob):
    cached_pso{ pso_blob }
{

}

ComputePSODescriptor::ComputePSODescriptor(D3DDataBlob const & cs, uint32_t node_mask):
    compute_shader{ cs },
    node_mask{ node_mask }
{
}

ComputePSODescriptor::ComputePSODescriptor(D3DDataBlob const & pso_blob):
    cached_pso{ pso_blob }
{
}
