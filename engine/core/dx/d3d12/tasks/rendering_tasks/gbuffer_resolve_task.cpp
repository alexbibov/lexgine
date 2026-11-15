#include "engine/core/globals.h"
#include "engine/core/engine_api.h"
#include "engine/core/global_settings.h"
#include "engine/core/rendering_configuration.h"
#include "engine/core/math/utility.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/descriptor_allocator.h"
#include "engine/core/dx/d3d12/basic_rendering_services.h"
#include "engine/core/dx/d3d12/resource_barrier_pack.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"
#include "engine/core/dx/d3d12/caches/pso_blob_cache.h"
#include "engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h"
#include "engine/core/dx/dxcompilation/shader_stage.h"
#include "engine/core/dx/d3d12/rendering/gbuffer.h"

#include "gbuffer_resolve_task.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;
using namespace lexgine::core::dx::d3d12::caches;


GBufferResolveTask::GBufferResolveTask(Globals& globals, BasicRenderingServices& rendering_services)
    : RenderingWork{ globals, "GBuffer resolve task", CommandType::direct }
    , m_globals{ globals }
    , m_device{ globals.device() }
    , m_basic_rendering_services{ rendering_services }
    , m_shader_function{ m_globals, dx::dxcompilation::ShaderFunctionRootUniformBuffers::base_values::None }
    , m_cmd_list_ptr{ addCommandList() }
{
    HLSLShaderBlobCache& hlsl_shader_blob_cache = globals.hlslShaderBlobCache();

    HLSLFileTranslationUnit translation_unit_vs{ m_globals, "resolve.vs", "resolve/resolve.vs.hlsl" };
    caches::HLSLShaderHandle vs_handle = hlsl_shader_blob_cache.createHLSLShaderBlobCompilationContract(
        translation_unit_vs, dx::dxcompilation::ShaderModel::model_60, dx::dxcompilation::ShaderType::vertex, "VSMain");

    HLSLFileTranslationUnit translation_unit_ps{ m_globals, "resolve.ps", "resolve/resolve.ps.hlsl" };
    caches::HLSLShaderHandle ps_handle = hlsl_shader_blob_cache.createHLSLShaderBlobCompilationContract(
        translation_unit_ps, dx::dxcompilation::ShaderModel::model_60, dx::dxcompilation::ShaderType::pixel, "PSMain");

    dx::dxcompilation::ShaderStage* p_vs_stage = m_shader_function.createShaderStage(vs_handle);
    dx::dxcompilation::ShaderStage* p_ps_stage = m_shader_function.createShaderStage(ps_handle);
    p_vs_stage->build();
    p_ps_stage->build();
    m_shader_function.collectInputResourceBindings();
    m_rs_handle = m_shader_function.buildInputResourceBindings();

    m_texture_table = m_shader_function.createDescriptorTable(
        m_shader_function.findDescriptorTable(dx::dxcompilation::BindingDomain::pass, DescriptorHeapType::cbv_srv_uav).value(),
        m_device.persistentDescriptorAllocator(DescriptorHeapType::cbv_srv_uav));
    m_sampler_table = m_shader_function.createDescriptorTable(
        m_shader_function.findDescriptorTable(dx::dxcompilation::BindingDomain::pass, DescriptorHeapType::sampler).value(),
        m_device.persistentDescriptorAllocator(DescriptorHeapType::sampler));
    m_shader_function.bindSampler(*m_sampler_table, "linear_sampler",
        FilterPack { MinificationFilter::linear, MagnificationFilter::linear, 1, WrapMode::clamp, WrapMode::clamp, WrapMode::clamp },
        math::Vector4f { 0.f });
}

void GBufferResolveTask::updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags, RenderingConfiguration const& rendering_configuration)
{
    if (update_flags.isSet(RenderingConfigurationUpdateFlags::base_values::color_format_changed))
    {
        PSOBlobCache& pso_blob_cache = m_globals.psoBlobCache();
        RootSignatureBlobCache& rs_blob_cache = m_globals.rootSignatureBlobCache();

        GraphicsPSODescriptor pso_descriptor{};
        pso_descriptor.rtv_formats[0] = rendering_configuration.color_buffer_format;
        pso_descriptor.num_render_targets = 1;
        pso_descriptor.dsv_format = DXGI_FORMAT_UNKNOWN;
        pso_descriptor.primitive_topology_type = PrimitiveTopologyType::triangle;
        pso_descriptor.node_mask = 1;
        pso_descriptor.vertex_shader = m_shader_function.getShaderStage(dx::dxcompilation::ShaderType::vertex)->getShaderBytecode();
        pso_descriptor.pixel_shader = m_shader_function.getShaderStage(dx::dxcompilation::ShaderType::pixel)->getShaderBytecode();

        m_pso_handle = pso_blob_cache.createGraphicsPSOBlobCompilationContract(pso_descriptor, m_rs_handle);
        pso_blob_cache.createPipelineStates();
        auto [pso, pso_status] = pso_blob_cache.getGraphicsPipelineState(m_pso_handle);
        (void) pso_status;
        m_pso = pso;
        auto [compiled_rs, rs_status] = rs_blob_cache.getNativeRootSignature(m_rs_handle);
        (void) rs_status;
        m_compiled_rs = compiled_rs;
    }
}

bool GBufferResolveTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    if (!m_gbuffer || !m_pso || !m_compiled_rs)
    {
        return true;
    }

    core::rendering::FrameContext const& frame_context = m_basic_rendering_services.frameContext(user_data);
    m_basic_rendering_services.beginRendering(*m_cmd_list_ptr, frame_context);

    m_cmd_list_ptr->setPipelineState(*m_pso);
    m_cmd_list_ptr->setRootSignature(*m_compiled_rs);

    m_basic_rendering_services.setDefaultResources(*m_cmd_list_ptr);
    m_basic_rendering_services.setDefaultViewport(*m_cmd_list_ptr);
    m_basic_rendering_services.setDefaultRenderingTarget(*m_cmd_list_ptr, frame_context);

    if (m_gbuffer->needsAlbedoResolve())
    {
        Resource const& albedo = m_gbuffer->albedo();
        Resource const& resolved_albedo = m_gbuffer->sampleableAlbedo();

        StaticResourceBarrierPack<2> to_resolve;
        to_resolve.addTransitionBarrier(&albedo, ResourceState::base_values::pixel_shader, ResourceState::base_values::resolve_source);
        to_resolve.addTransitionBarrier(&resolved_albedo, ResourceState::base_values::pixel_shader, ResourceState::base_values::resolve_destination);
        to_resolve.applyBarriers(*m_cmd_list_ptr);

        m_cmd_list_ptr->resolveSubresource(resolved_albedo, 0, albedo, 0, DXGI_FORMAT_R11G11B10_FLOAT);

        StaticResourceBarrierPack<2> from_resolve;
        from_resolve.addTransitionBarrier(&albedo, ResourceState::base_values::resolve_source, ResourceState::base_values::pixel_shader);
        from_resolve.addTransitionBarrier(&resolved_albedo, ResourceState::base_values::resolve_destination, ResourceState::base_values::pixel_shader);
        from_resolve.applyBarriers(*m_cmd_list_ptr);
    }

    m_shader_function.bindTexture(*m_texture_table, "gbuffer_albedo", m_gbuffer->sampleableAlbedo());
    m_shader_function.setDescriptorTable(*m_cmd_list_ptr, *m_texture_table);
    m_shader_function.setDescriptorTable(*m_cmd_list_ptr, *m_sampler_table);

    m_cmd_list_ptr->inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle_list);
    m_cmd_list_ptr->drawInstanced(3, 1, 0, 0);

    return true;
}
