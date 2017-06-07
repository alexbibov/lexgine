#include "pipeline_state.h"
#include "d3d12_tools.h"

using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;

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
    m_pipeline_state->GetCachedBlob(&p_blob);
    return D3DDataBlob{ p_blob };
}

PipelineState::PipelineState(Device& device, D3DDataBlob const& serialized_root_signature, GraphicsPSODescriptor const& pso_descriptor, D3DDataBlob const& cached_pso):
    m_device{ device }
{
    ComPtr<ID3D12RootSignature> root_signature = device.createRootSignature(serialized_root_signature, pso_descriptor.node_mask);

    #ifdef D3D12DEBUG
    root_signature->SetName(misc::asciiStringToWstring(getStringName() + "_root_signature").c_str());
    #endif


    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;

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

    D3D12_SO_DECLARATION_ENTRY *p_so_declaration_entries = new D3D12_SO_DECLARATION_ENTRY[so_desc.NumEntries];
    uint32_t so_declaration_entry_idx = 0;
    for (auto p = pso_descriptor.stream_output.so_declarations.begin(); p != pso_descriptor.stream_output.so_declarations.end(); ++p, ++so_declaration_entry_idx)
    {
        p_so_declaration_entries[so_declaration_entry_idx].Stream = p->stream();
        p_so_declaration_entries[so_declaration_entry_idx].SemanticName = p->name().c_str();
        p_so_declaration_entries[so_declaration_entry_idx].SemanticIndex = p->nameIndex();
        auto element_components = p->outputComponents();
        p_so_declaration_entries[so_declaration_entry_idx].StartComponent = element_components.first;
        p_so_declaration_entries[so_declaration_entry_idx].ComponentCount = element_components.second;
        p_so_declaration_entries[so_declaration_entry_idx].OutputSlot = p->slot();
    }
    so_desc.pSODeclaration = p_so_declaration_entries;

    so_desc.NumStrides = static_cast<UINT>(pso_descriptor.stream_output.buffer_strides.size());

    UINT *p_buffer_strides = new UINT[so_desc.NumStrides];
    uint32_t buffer_stride_idx = 0;
    for (auto p = pso_descriptor.stream_output.buffer_strides.begin(); p != pso_descriptor.stream_output.buffer_strides.end(); ++p, ++buffer_stride_idx)
        p_buffer_strides[buffer_stride_idx] = *p;
    so_desc.pBufferStrides = p_buffer_strides;

    so_desc.RasterizedStream = pso_descriptor.stream_output.rasterized_stream;

    desc.StreamOutput = so_desc;


    // Define blend state
    D3D12_BLEND_DESC blend_desc;
    blend_desc.AlphaToCoverageEnable = pso_descriptor.blend_state.alphaToCoverageEnable;
    blend_desc.IndependentBlendEnable = pso_descriptor.blend_state.independentBlendEnable;
    for (uint8_t i = 0U; i < sizeof(blend_desc.RenderTarget) / sizeof(D3D12_RENDER_TARGET_BLEND_DESC); ++i)
    {
        blend_desc.RenderTarget[i].BlendEnable = pso_descriptor.blend_state.render_target_blend_descriptor[i].isEnabled();
        blend_desc.RenderTarget[i].LogicOpEnable = pso_descriptor.blend_state.render_target_blend_descriptor[i].isLogicalOperationEnabled();
        auto srt_blend_factors = pso_descriptor.blend_state.render_target_blend_descriptor[i].getSourceBlendFactors();
        auto dst_blend_factors = pso_descriptor.blend_state.render_target_blend_descriptor[i].getDestinationBlendFactors();
        auto blend_op = pso_descriptor.blend_state.render_target_blend_descriptor[i].getBlendOperation();
        blend_desc.RenderTarget[i].SrcBlend = d3d12Convert(srt_blend_factors.first);
        blend_desc.RenderTarget[i].DestBlend = d3d12Convert(dst_blend_factors.first);
        blend_desc.RenderTarget[i].BlendOp = d3d12Convert(blend_op.first);
        blend_desc.RenderTarget[i].SrcBlendAlpha = d3d12Convert(srt_blend_factors.second);
        blend_desc.RenderTarget[i].DestBlendAlpha = d3d12Convert(dst_blend_factors.second);
        blend_desc.RenderTarget[i].BlendOpAlpha = d3d12Convert(blend_op.second);
        blend_desc.RenderTarget[i].LogicOp = d3d12Convert(pso_descriptor.blend_state.render_target_blend_descriptor[i].getBlendLogicalOperation());
        blend_desc.RenderTarget[i].RenderTargetWriteMask = pso_descriptor.blend_state.render_target_blend_descriptor[i].getColorWriteMask().getValue();
    }
    desc.BlendState = blend_desc;

    desc.SampleMask = pso_descriptor.sample_mask;


    // Define rasterizer state
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


    // Define depth-stencil state
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


    // Vertex attributes input layout specification
    D3D12_INPUT_LAYOUT_DESC input_layout_desc;
    input_layout_desc.NumElements = static_cast<UINT>(pso_descriptor.vertex_attributes.size());
    D3D12_INPUT_ELEMENT_DESC* p_input_element_descs = new D3D12_INPUT_ELEMENT_DESC[input_layout_desc.NumElements];
    input_layout_desc.pInputElementDescs = p_input_element_descs;

    size_t input_element_desc_idx = 0;
    for (auto p = pso_descriptor.vertex_attributes.begin(); p != pso_descriptor.vertex_attributes.end(); ++p, ++input_element_desc_idx)
    {
        p_input_element_descs[input_element_desc_idx].SemanticName = (**p).name().c_str();
        p_input_element_descs[input_element_desc_idx].SemanticIndex = (**p).name_index();
        p_input_element_descs[input_element_desc_idx].Format = (**p).format<EngineAPI::Direct3D12>();
        p_input_element_descs[input_element_desc_idx].InputSlot = (**p).id();
        p_input_element_descs[input_element_desc_idx].AlignedByteOffset = (**p).capacity();
        p_input_element_descs[input_element_desc_idx].InputSlotClass = (**p).type() == AbstractVertexAttributeSpecification::specification_type::per_instance ?
            D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        p_input_element_descs[input_element_desc_idx].InstanceDataStepRate = (**p).instancing_rate();
    }


    // Primitive restart configuration
    desc.IBStripCutValue = pso_descriptor.primitive_restart ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE::D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE::D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;


    // Primitive topology configuration
    desc.PrimitiveTopologyType = d3d12Convert(pso_descriptor.primitive_topology);


    // Miscellaneous
    desc.NumRenderTargets = pso_descriptor.num_render_targets;
    for (uint8_t i = 0; i < 8; ++i) desc.RTVFormats[i] = pso_descriptor.rtv_formats[i];
    desc.DSVFormat = pso_descriptor.dsv_format;
    desc.SampleDesc.Count = pso_descriptor.multi_sampling_format.count;
    desc.SampleDesc.Quality = pso_descriptor.multi_sampling_format.quality;
    desc.NodeMask = pso_descriptor.node_mask;

    if (cached_pso)
        desc.CachedPSO = D3D12_CACHED_PIPELINE_STATE{ cached_pso.data(), cached_pso.size() };

    #ifdef D3D12DEBUG
    desc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
    #else
    desc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
    #endif

    LEXGINE_ERROR_LOG(
        this, 
        m_device.native()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state)), 
        S_OK
    );

    delete[] p_so_declaration_entries;
    delete[] p_buffer_strides;
    delete[] p_input_element_descs;
}

PipelineState::PipelineState(Device & device, D3DDataBlob const & serialized_root_signature, ComputePSODescriptor const & pso_descriptor, D3DDataBlob const& cached_pso):
    m_device{ device }
{
    ID3D12RootSignature* p_root_signature;
    m_device.native()->CreateRootSignature(pso_descriptor.node_mask, serialized_root_signature.data(), serialized_root_signature.size(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&p_root_signature));

    #ifdef D3D12DEBUG
    p_root_signature->SetName(misc::asciiStringToWstring(getStringName()+"_root_signature").c_str());
    #endif


    D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
    desc.pRootSignature = p_root_signature;
    desc.CS = D3D12_SHADER_BYTECODE{ pso_descriptor.compute_shader.data(), pso_descriptor.compute_shader.size() };
    desc.NodeMask = pso_descriptor.node_mask;

    if (cached_pso)
        desc.CachedPSO = D3D12_CACHED_PIPELINE_STATE{ cached_pso.data(), cached_pso.size() };

    #ifdef D3D12DEBUG
    desc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
    #else
    desc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
    #endif

    LEXGINE_ERROR_LOG(
        this, 
        m_device.native()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state)), 
        S_OK
    );

    p_root_signature->Release();
}

void PipelineState::setStringName(std::string const & entity_string_name)
{
    Entity::setStringName(entity_string_name);
    m_pipeline_state->SetName(misc::asciiStringToWstring(entity_string_name).c_str());
}

GraphicsPSODescriptor::GraphicsPSODescriptor(VertexAttributeSpecificationList const& vertex_attribute_specs, D3DDataBlob const& vs, D3DDataBlob const& ps,
    DXGI_FORMAT rt_format, uint32_t node_mask, PrimitiveTopology topology) :
    vertex_shader{ vs },
    pixel_shader{ ps },
    vertex_attributes{ vertex_attribute_specs },
    primitive_restart{ false },
    primitive_topology{ topology },
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
