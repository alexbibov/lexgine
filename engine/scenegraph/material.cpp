#include <bit>
#include <type_traits>

#include <engine/core/dx/d3d12/command_list.h>

#include <engine/core/globals.h>
#include <engine/core/global_settings.h>
#include <engine/core/dx/d3d12/device.h>
#include <engine/core/dx/d3d12/basic_rendering_services.h>
#include <engine/core/dx/d3d12/caches/pso_blob_cache.h>
#include <engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h>
#include <engine/core/dx/d3d12/caches/root_signature_blob_cache.h>
#include <engine/core/dx/d3d12/unordered_srv_table_allocation_manager.h>
#include <engine/core/misc/datetime.h>
#include <engine/core/dx/d3d12/dx_resource_factory.h>
#include <engine/core/dx/dxcompilation/shader_stage.h>
#include <engine/conversion/texture_converter.h>
#include "engine/core/misc/hashes/xxhash64.h"

#include "image.h"

#include "material.h"

namespace lexgine::scenegraph
{

#pragma region MaterialPSOCompilationContext
MaterialPSOCompilationContext::MaterialPSOCompilationContext(core::VertexAttributeSpecificationList const& va_list)
    : va_list { va_list }
    , depth_stencil_format { DXGI_FORMAT_D32_FLOAT }
{
    std::fill(render_target_formats, render_target_formats + sizeof(render_target_formats) / sizeof(DXGI_FORMAT), DXGI_FORMAT_UNKNOWN);
    render_target_formats[0] = DXGI_FORMAT_R11G11B10_FLOAT; // Albedo (RGB)
    render_target_formats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normals (RG), metallic (LS-bits) and roughness (MS-bits) of B, A - red component of emission
    render_target_formats[2] = DXGI_FORMAT_R16G16_FLOAT; // Emission intensity (GB components)
}
#pragma endregion

#pragma region MaterialStaticState
MaterialStaticState::MaterialStaticState(
    core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
    MaterialPSOCompilationContext const& context,
    MaterialShaderDesc const& shader_desc
)
    : m_basic_rendering_services{ basic_rendering_services }
    , m_shader_function{ std::make_unique<core::dx::dxcompilation::ShaderFunction>(m_basic_rendering_services.globals(), core::dx::dxcompilation::ShaderFunctionRootUniformBuffers::base_values::All) }
    , m_material_parameters_ub_name{ shader_desc.material_parameters_uniform_buffer_name }
    , m_scene_parameters_ub_name{ shader_desc.scene_parameters_uniform_buffer_name }
{
    m_pso_descriptor.stream_output = context.stream_output;
    m_pso_descriptor.blend_state = context.blend_state;
    m_pso_descriptor.rasterization_descriptor = context.rasterization_descriptor;
    m_pso_descriptor.depth_stencil_descriptor = context.depth_stencil_descriptor;
    m_pso_descriptor.vertex_attributes = context.va_list;
    m_pso_descriptor.primitive_restart = false;
    m_pso_descriptor.primitive_topology_type = core::PrimitiveTopologyType::triangle;

    for (m_pso_descriptor.num_render_targets = 0;
        m_pso_descriptor.num_render_targets < 8;
        ++m_pso_descriptor.num_render_targets)
    {
        if (context.render_target_formats[m_pso_descriptor.num_render_targets] == DXGI_FORMAT_UNKNOWN)
        {
            break;
        }
        m_pso_descriptor.rtv_formats[m_pso_descriptor.num_render_targets] = context.render_target_formats[m_pso_descriptor.num_render_targets];
    }
    m_pso_descriptor.dsv_format = context.depth_stencil_format;

    auto* p_global_settings = m_shader_function->globals().get<core::GlobalSettings>();
    core::MSAAMode msaa_mode = p_global_settings->msaaMode();
    auto* p_device = m_shader_function->globals().get<core::dx::d3d12::Device>();
    uint32_t num_quality_levels = std::numeric_limits<uint32_t>::max();
    for (int i = 0; i < static_cast<int>(m_pso_descriptor.num_render_targets); ++i)
    {
        core::dx::d3d12::FeatureMultisampleQualityLevels quality_level = p_device->queryFeatureQualityLevels(m_pso_descriptor.rtv_formats[i], static_cast<uint32_t>(msaa_mode));
        num_quality_levels = (std::min)(num_quality_levels, quality_level.num_quality_levels);
    }
    if (num_quality_levels == 0 || num_quality_levels == std::numeric_limits<uint32_t>::max())
    {
        msaa_mode = core::MSAAMode::none;
        num_quality_levels = 1;
    }
    m_pso_descriptor.multi_sampling_format = core::MultiSamplingFormat{ static_cast<uint32_t>(msaa_mode), num_quality_levels - 1 };

    assert(shader_desc.vertex_shader.p_internal && shader_desc.pixel_shader.p_internal);
    m_shader_function->createShaderStage(shader_desc.vertex_shader);
    m_shader_function->createShaderStage(shader_desc.pixel_shader);
    if (shader_desc.hull_shader.p_internal)
    {
        m_shader_function->createShaderStage(shader_desc.hull_shader);
    }
    if (shader_desc.domain_shader.p_internal)
    {
        m_shader_function->createShaderStage(shader_desc.domain_shader);
    }
    if (shader_desc.geometry_shader.p_internal)
    {
        m_shader_function->createShaderStage(shader_desc.geometry_shader);
    }
}

void MaterialStaticState::buildPipeline()
{
    m_rs_handle = m_shader_function->buildBindingSignature();

    {
        // Setup shader function resources
        core::dx::d3d12::Device& device = *m_basic_rendering_services.globals().get<core::dx::d3d12::Device>();
        core::dx::d3d12::DescriptorHeap& resource_descriptor_heap = m_basic_rendering_services.dxResources().retrieveDescriptorHeap(device, core::dx::d3d12::DescriptorHeapType::cbv_srv_uav, 0);
        core::dx::d3d12::UnorderedSRVTableAllocationManager& allocator = m_basic_rendering_services.dxResources().retrieveBindlessSRVAllocationManager(resource_descriptor_heap);
        m_shader_function->assignResourceDescriptors(core::dx::dxcompilation::ShaderFunction::ShaderInputKind::srv, 0, allocator);
        m_material_parameters_cb_reflection = m_shader_function->getShaderStage(lexgine::core::dx::dxcompilation::ShaderType::pixel)->buildConstantBufferReflection(m_material_parameters_ub_name);
        m_scene_parameters_cb_reflection = m_shader_function->getShaderStage(lexgine::core::dx::dxcompilation::ShaderType::vertex)->buildConstantBufferReflection(m_scene_parameters_ub_name);
    }

    m_pso_descriptor.vertex_shader = m_shader_function->getShaderStage(core::dx::dxcompilation::ShaderType::vertex)->getShaderBytecode();
    m_pso_descriptor.pixel_shader = m_shader_function->getShaderStage(core::dx::dxcompilation::ShaderType::pixel)->getShaderBytecode();
    if (auto* p_stage = m_shader_function->getShaderStage(core::dx::dxcompilation::ShaderType::hull))
        m_pso_descriptor.hull_shader = p_stage->getShaderBytecode();
    if (auto* p_stage = m_shader_function->getShaderStage(core::dx::dxcompilation::ShaderType::domain))
        m_pso_descriptor.domain_shader = p_stage->getShaderBytecode();
    if (auto* p_stage = m_shader_function->getShaderStage(core::dx::dxcompilation::ShaderType::geometry))
        m_pso_descriptor.geometry_shader = p_stage->getShaderBytecode();
    m_pso_descriptor.invalidateHash();

    core::dx::d3d12::caches::PSOBlobCache& pso_blob_cache = *m_basic_rendering_services.globals().get<core::dx::d3d12::caches::PSOBlobCache>();
    m_pso_handle = pso_blob_cache.createGraphicsPSOBlobCompilationContract(m_pso_descriptor, m_rs_handle);
}

void MaterialStaticState::bindMaterialParameters(
    core::dx::d3d12::CommandList& target_command_list, 
    core::dx::d3d12::ConstantBufferDataMapper& data_mapper
) const
{
    auto allocation = m_basic_rendering_services.constantDataStream().allocateAndUpdate(data_mapper);
    m_shader_function->bindRootConstantBuffer(
        target_command_list, 
        core::dx::dxcompilation::ShaderFunctionConstantBufferRootIds::material_uniforms, 
        allocation->virtualGpuAddress()
    );
}

void MaterialStaticState::bindObjectParameters(
    core::dx::d3d12::CommandList& target_command_list,
    core::dx::d3d12::ConstantBufferDataMapper& data_mapper
) const
{
    auto allocation = m_basic_rendering_services.constantDataStream().allocateAndUpdate(data_mapper);
    m_shader_function->bindRootConstantBuffer(
        target_command_list,
        core::dx::dxcompilation::ShaderFunctionConstantBufferRootIds::object_uniforms,
        allocation->virtualGpuAddress()
    );
}

void MaterialStaticState::bindSceneParameters(
    core::dx::d3d12::CommandList& target_command_list, 
    core::dx::d3d12::ConstantBufferDataMapper& data_mapper
) const
{
    auto allocation = m_basic_rendering_services.constantDataStream().allocateAndUpdate(data_mapper);
    m_shader_function->bindRootConstantBuffer(
        target_command_list,
        core::dx::dxcompilation::ShaderFunctionConstantBufferRootIds::scene_uniforms,
        allocation->virtualGpuAddress()
    );
}


bool MaterialStaticState::operator==(MaterialStaticState const& other) const
{
    return m_pso_handle == other.m_pso_handle;
}

const char* const Material::c_material_parameters_uniform_buffer_name = "MaterialUniforms";


Material::Material(MaterialStaticState const& material_static_state)
    : m_material_static_state{ material_static_state }
    , m_material_parameters_cb_data_mapper{ material_static_state.getMaterialParametersUniformBufferReflection() }
{
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("emissive_factor", m_emissive_factor);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("alpha_mode", static_cast<unsigned int>(m_alpha_mode));
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("alpha_cutoff", m_alpha_cutoff);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("is_double_sided", m_is_double_sided);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("base_color_factor", m_base_color_factor);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("metallic_factor", m_metallic_factor);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("roughness_factor", m_roughness_factor);

    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("normal_tex_index", m_normal_texture_binding_id);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("srv_occlusion", m_occlusion_texture_binding_id);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("emissive_tex_index", m_emissive_texture_binding_id);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("albedo_tex_index", m_base_color_texture_binding_id);
    m_material_parameters_cb_data_mapper.addOrUpdateDataBinding("mr_tex_index", m_metallic_roughness_texture_binding_id);
}

void Material::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);
}

void Material::setMetallicRoughness(MetallicRoughness const& value)
{
    m_base_color_factor = value.base_color_factor;
    m_metallic_factor = value.metallic_factor;
    m_roughness_factor = value.roughness_factor;
    
    assert(value.p_base_color->p_texture_conversion_task->getStatus() == lexgine::conversion::TextureConversionStatus::completed);
    lexgine::conversion::TextureUploadWork* p_base_color_texture_upload_work = value.p_base_color->p_texture_conversion_task->getUploadWork();
    assert(p_base_color_texture_upload_work->isCompleted());
    if (core::dx::dxcompilation::BindingResult binding_result =
        m_material_static_state.getShaderStage(core::dx::dxcompilation::ShaderType::pixel)->bindTexture("material_textures", p_base_color_texture_upload_work->resource()))
    {
        m_base_color_texture_binding_id = binding_result.binding_register;
    }
    assert(value.p_metallic_roughness->p_texture_conversion_task->getStatus() == lexgine::conversion::TextureConversionStatus::completed);
    lexgine::conversion::TextureUploadWork* p_metallic_roughness_texture_upload_work = value.p_metallic_roughness->p_texture_conversion_task->getUploadWork();
    assert(p_metallic_roughness_texture_upload_work->isCompleted());
    if (core::dx::dxcompilation::BindingResult binding_result 
        = m_material_static_state.getShaderStage(core::dx::dxcompilation::ShaderType::pixel)->bindTexture("material_textures", p_metallic_roughness_texture_upload_work->resource()))
    {
        m_metallic_roughness_texture_binding_id = binding_result.binding_register;
    }
}

void Material::setNormalTexture(Texture* p_texture)
{
    assert(p_texture->p_texture_conversion_task->getStatus() == lexgine::conversion::TextureConversionStatus::completed);
    lexgine::conversion::TextureUploadWork* p_texture_upload_work = p_texture->p_texture_conversion_task->getUploadWork();
    assert(p_texture_upload_work->isCompleted());
    if (core::dx::dxcompilation::BindingResult binding_result =
        m_material_static_state.getShaderStage(core::dx::dxcompilation::ShaderType::pixel)->bindTexture("material_textures", p_texture_upload_work->resource()))
    {
        m_normal_texture_binding_id = binding_result.binding_register;
    }
    
}

void Material::setOcclusionTexture(Texture* p_texture)
{
    assert(p_texture->p_texture_conversion_task->getStatus() == lexgine::conversion::TextureConversionStatus::completed);
    lexgine::conversion::TextureUploadWork* p_texture_upload_work = p_texture->p_texture_conversion_task->getUploadWork();
    assert(p_texture_upload_work->isCompleted());
    if (core::dx::dxcompilation::BindingResult binding_result 
        = m_material_static_state.getShaderStage(core::dx::dxcompilation::ShaderType::pixel)->bindTexture("material_textures", p_texture_upload_work->resource()))
    {
        m_occlusion_texture_binding_id = binding_result.binding_register;
    }
}

void Material::setEmissiveTexture(Texture* p_texture)
{
    assert(p_texture->p_texture_conversion_task->getStatus() == lexgine::conversion::TextureConversionStatus::completed);
    lexgine::conversion::TextureUploadWork* p_texture_upload_work = p_texture->p_texture_conversion_task->getUploadWork();
    assert(p_texture_upload_work->isCompleted());
    if (core::dx::dxcompilation::BindingResult binding_result 
        = m_material_static_state.getShaderStage(core::dx::dxcompilation::ShaderType::pixel)->bindTexture("material_textures", p_texture_upload_work->resource()))
    {
        m_emissive_texture_binding_id = binding_result.binding_register;
    }
}

#pragma endregion

}
