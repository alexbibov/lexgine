#include <type_traits>

#include <engine/core/dx/d3d12/command_list.h>

#include <engine/core/globals.h>
#include <engine/core/global_settings.h>
#include <engine/core/dx/d3d12/device.h>
#include <engine/core/dx/d3d12/basic_rendering_services.h>
#include <engine/core/dx/d3d12/tasks/root_signature_compilation_task.h>
#include <engine/core/dx/d3d12/tasks/pso_compilation_task.h>
#include <engine/core/dx/d3d12/task_caches/pso_compilation_task_cache.h>
#include <engine/core/dx/d3d12/unordered_srv_table_allocation_manager.h>
#include <engine/core/dx/d3d12/dx_resource_factory.h>
#include <engine/core/dx/dxcompilation/shader_stage.h>
#include <engine/conversion/texture_converter.h>

#include "image.h"

#include "material.h"

namespace lexgine::scenegraph
{

MaterialPSOCompilationContext::MaterialPSOCompilationContext(core::VertexAttributeSpecificationList const& va_list)
    : va_list { va_list }
    , depth_stencil_format { DXGI_FORMAT_D32_FLOAT }
{
    std::fill(render_target_formats, render_target_formats + sizeof(render_target_formats) / sizeof(DXGI_FORMAT), DXGI_FORMAT_UNKNOWN);
    render_target_formats[0] = DXGI_FORMAT_R11G11B10_FLOAT; // Albedo (RGB)
    render_target_formats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normals (RG), metallic (LS-bits) and roughness (MS-bits) of B, A - red component of emission
    render_target_formats[2] = DXGI_FORMAT_R16G16_FLOAT; // Emission intensity (BA components)
}


MaterialAssemblyTask::MaterialAssemblyTask(
    core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
    MaterialPSOCompilationContext const& context,
    MaterialShaderDesc const& shaders
)
    : m_basic_rendering_services{ basic_rendering_services }
    , m_shader_function{ m_basic_rendering_services.globals(), core::dx::dxcompilation::ShaderFunctionRootUniformBuffers::base_values::All }
    , m_material_parameters_ub_name{ shaders.material_parameters_uniform_buffer_name }
    , m_scene_parameters_ub_name{ shaders.scene_parameters_uniform_buffer_name }
{
    core::dx::d3d12::task_caches::PSOCompilationTaskCache& pso_compilation_task_cache = *m_shader_function.globals().get<core::dx::d3d12::task_caches::PSOCompilationTaskCache>();
    
    assert(shaders.p_vertex_shader_compilation_task && shaders.p_pixel_shader_compilation_task);
    core::dx::d3d12::tasks::HLSLCompilationTask* p_vertex_shader_compilation_task = shaders.p_vertex_shader_compilation_task;
    this->addDependency(*p_vertex_shader_compilation_task);
    core::dx::d3d12::tasks::HLSLCompilationTask* p_pixel_shader_compilation_task = shaders.p_pixel_shader_compilation_task;
    this->addDependency(*p_pixel_shader_compilation_task);
    m_shader_function.createShaderStage(shaders.p_vertex_shader_compilation_task);
    m_shader_function.createShaderStage(shaders.p_pixel_shader_compilation_task);

    if (core::dx::d3d12::tasks::HLSLCompilationTask* p_hull_shader_compilation_task = shaders.p_hull_shader_compilation_task)
    {
        this->addDependency(*p_hull_shader_compilation_task);
        m_shader_function.createShaderStage(shaders.p_hull_shader_compilation_task);
    }
    if (core::dx::d3d12::tasks::HLSLCompilationTask* p_domain_shader_compilation_task = shaders.p_domain_shader_compilation_task)
    {
        this->addDependency(*p_domain_shader_compilation_task);
        m_shader_function.createShaderStage(shaders.p_domain_shader_compilation_task);
    }
    if (core::dx::d3d12::tasks::HLSLCompilationTask* p_geometry_shader_compilation_task = shaders.p_geometry_shader_compilation_task)
    {
        this->addDependency(*p_geometry_shader_compilation_task);
        m_shader_function.createShaderStage(shaders.p_geometry_shader_compilation_task);
    }

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

    core::GlobalSettings* p_global_settings = m_shader_function.globals().get<core::GlobalSettings>();
    core::MSAAMode msaa_mode = p_global_settings->msaaMode();
    core::dx::d3d12::Device* p_device = m_shader_function.globals().get<core::dx::d3d12::Device>();
    
    uint32_t msaa_quality_level = std::numeric_limits<uint32_t>::max();
    for (int i = 0; i < static_cast<int>(m_pso_descriptor.num_render_targets); ++i)
    {
        core::dx::d3d12::FeatureMultisampleQualityLevels quality_level = p_device->queryFeatureQualityLevels(m_pso_descriptor.rtv_formats[i], static_cast<uint32_t>(msaa_mode));
        msaa_quality_level = (std::min)(msaa_quality_level, quality_level.num_quality_levels);
    }

    if (msaa_quality_level == 0 || msaa_quality_level == std::numeric_limits<uint32_t>::max())
    {
        msaa_mode = core::MSAAMode::none;
        msaa_quality_level = 1;
    }
    m_pso_descriptor.multi_sampling_format = core::MultiSamplingFormat{ static_cast<uint32_t>(msaa_mode), msaa_quality_level };

    m_pso_compilation_task = pso_compilation_task_cache.findOrCreateTask(m_shader_function.globals(), m_pso_descriptor, getStringName() + "__PSO", 0);
}

bool MaterialAssemblyTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    m_root_signature_compilation_task = m_shader_function.buildBindingSignature();

    {
        // Setup shader function resources
        core::dx::d3d12::Device& device = *m_basic_rendering_services.globals().get<core::dx::d3d12::Device>();
        core::dx::d3d12::DescriptorHeap& resource_descriptor_heap = m_basic_rendering_services.dxResources().retrieveDescriptorHeap(device, core::dx::d3d12::DescriptorHeapType::cbv_srv_uav, 0);
        core::dx::d3d12::UnorderedSRVTableAllocationManager& allocator = m_basic_rendering_services.dxResources().retrieveBindlessSRVAllocationManager(resource_descriptor_heap);
        m_shader_function.assignResourceDescriptors(core::dx::dxcompilation::ShaderFunction::ShaderInputKind::srv, 0, allocator);

        m_material_parameters_cb_reflection = m_shader_function.getShaderStage(lexgine::core::dx::dxcompilation::ShaderType::pixel)->buildConstantBufferReflection(m_material_parameters_ub_name);
        m_scene_parameters_cb_reflection = m_shader_function.getShaderStage(lexgine::core::dx::dxcompilation::ShaderType::pixel)->buildConstantBufferReflection(m_scene_parameters_ub_name);
    }

    if (!m_root_signature_compilation_task->execute(worker_id))
    {
        return false;
    }

    m_pso_compilation_task->setVertexShaderCompilationTask(m_shader_function.getShaderStage(core::dx::dxcompilation::ShaderType::vertex)->getTask());
    m_pso_compilation_task->setPixelShaderCompilationTask(m_shader_function.getShaderStage(core::dx::dxcompilation::ShaderType::pixel)->getTask());
    if (auto* p_compilation_task = m_shader_function.getShaderStage(core::dx::dxcompilation::ShaderType::hull)->getTask())
    {
        m_pso_compilation_task->setHullShaderCompilationTask(p_compilation_task);
    }
    if (auto* p_compilation_task = m_shader_function.getShaderStage(core::dx::dxcompilation::ShaderType::domain)->getTask()) {
        m_pso_compilation_task->setDomainShaderCompilationTask(p_compilation_task);
    }
    if (auto* p_compilation_task = m_shader_function.getShaderStage(core::dx::dxcompilation::ShaderType::geometry)->getTask()) {
        m_pso_compilation_task->setGeometryShaderCompilationTask(p_compilation_task);
    }
    m_pso_compilation_task->setRootSignatureCompilationTask(m_root_signature_compilation_task);

    return m_pso_compilation_task->execute(worker_id);
}

void MaterialAssemblyTask::bindMaterialParameters(
    core::dx::d3d12::CommandList& target_command_list, 
    core::dx::d3d12::ConstantBufferDataMapper& data_mapper
)
{
    auto allocation = m_basic_rendering_services.constantDataStream().allocateAndUpdate(data_mapper);
    m_shader_function.bindRootConstantBuffer(
        target_command_list, 
        core::dx::dxcompilation::ShaderFunctionConstantBufferRootIds::material_uniforms, 
        allocation->virtualGpuAddress()
    );
}

void MaterialAssemblyTask::bindSceneParameters(
    core::dx::d3d12::CommandList& target_command_list, 
    core::dx::d3d12::ConstantBufferDataMapper& data_mapper
)
{
    auto allocation = m_basic_rendering_services.constantDataStream().allocateAndUpdate(data_mapper);
    m_shader_function.bindRootConstantBuffer(
        target_command_list,
        core::dx::dxcompilation::ShaderFunctionConstantBufferRootIds::scene_uniforms,
        allocation->virtualGpuAddress()
    );
}



const char* const Material::c_material_parameters_uniform_buffer_name = "MaterialUniforms";


Material::Material(MaterialAssemblyTask& material_assembly_task)
    : m_material_assembly{ material_assembly_task }
    , m_material_parameters_cb_data_mapper{ material_assembly_task.getMaterialParametersUniformBufferReflection() }
{
    m_material_parameters_cb_data_mapper.addDataBinding("v3_emissive_factor", m_emissive_factor);
    m_material_parameters_cb_data_mapper.addDataBinding("i_alpha_mode", static_cast<int>(m_alpha_mode));
    m_material_parameters_cb_data_mapper.addDataBinding("f_alpha_cutoff", m_alpha_cutoff);
    m_material_parameters_cb_data_mapper.addDataBinding("b_is_double_sided", m_is_double_sided);
    m_material_parameters_cb_data_mapper.addDataBinding("v4_base_color_factor", m_base_color_factor);
    m_material_parameters_cb_data_mapper.addDataBinding("f_metallic_factor", m_metallic_factor);
    m_material_parameters_cb_data_mapper.addDataBinding("f_roughness_factor", m_roughness_factor);

    m_material_parameters_cb_data_mapper.addDataBinding("srv_normal", m_normal_texture_binding_id);
    m_material_parameters_cb_data_mapper.addDataBinding("srv_occlusion", m_occlusion_texture_binding_id);
    m_material_parameters_cb_data_mapper.addDataBinding("srv_emissive", m_emissive_texture_binding_id);
    m_material_parameters_cb_data_mapper.addDataBinding("srv_base_color", m_base_color_texture_binding_id);
    m_material_parameters_cb_data_mapper.addDataBinding("srv_metallic_roughness", m_metallic_roughness_texture_binding_id);
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
        m_material_assembly.getPixelShaderStage()->bindTexture("textures", p_base_color_texture_upload_work->resource()))
    {
        m_base_color_texture_binding_id = binding_result.binding_register;
    }

    assert(value.p_metallic_roughness->p_texture_conversion_task->getStatus() == lexgine::conversion::TextureConversionStatus::completed);
    lexgine::conversion::TextureUploadWork* p_metallic_roughness_texture_upload_work = value.p_metallic_roughness->p_texture_conversion_task->getUploadWork();
    assert(p_metallic_roughness_texture_upload_work->isCompleted());
    if (core::dx::dxcompilation::BindingResult binding_result 
        = m_material_assembly.getPixelShaderStage()->bindTexture("textures", p_metallic_roughness_texture_upload_work->resource())) 
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
        m_material_assembly.getPixelShaderStage()->bindTexture("textures", p_texture_upload_work->resource()))
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
        = m_material_assembly.getPixelShaderStage()->bindTexture("textures", p_texture_upload_work->resource())) 
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
        = m_material_assembly.getPixelShaderStage()->bindTexture("textures", p_texture_upload_work->resource())) 
    {
        m_emissive_texture_binding_id = binding_result.binding_register;
    }
}

void Material::bindMaterialConstants(core::dx::d3d12::CommandList& target_command_list)
{
    m_material_assembly.bindMaterialParameters(target_command_list, m_material_parameters_cb_data_mapper);
}


}