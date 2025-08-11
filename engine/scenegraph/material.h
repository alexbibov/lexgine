#ifndef LEXGINE_SCENEGRAPH_PBR_MATERIAL_H

#include <memory>
#include <vector>
#include <string>

#include <engine/core/math/vector_types.h>
#include <engine/core/dx/dxcompilation/shader_function.h>
#include <engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h>
#include <engine/core/dx/d3d12/constant_buffer_reflection.h>
#include <engine/core/dx/d3d12/constant_buffer_data_mapper.h>
#include <engine/core/dx/d3d12/pipeline_state.h>
#include <engine/core/stream_output.h>
#include <engine/core/vertex_attributes.h>
#include <engine/core/math/vector_types.h>
#include <engine/core/concurrency/schedulable_task.h>
#include <engine/core/dx/dxcompilation/shader_function.h>
#include <engine/conversion/lexgine_conversion_fwd.h>

#include "lexgine_scenegraph_fwd.h"
#include "engine/scenegraph/class_names.h"
#include "image.h"

namespace lexgine::scenegraph {

struct Texture {
    Image image;
    int sampler_id;
    lexgine::conversion::TextureConversionTask const* p_texture_conversion_task;
};

struct TextureInfo {
    std::string textureName;
    std::vector<glm::vec2> uvCoordinates;
    std::unique_ptr<Image> texture;
};

enum class AlphaMode
{
    opaque = 0,
    mask,
    blend
};

struct MaterialPSOCompilationContext
{
    core::VertexAttributeSpecificationList va_list;
    DXGI_FORMAT render_target_formats[8];
    DXGI_FORMAT depth_stencil_format;
    core::BlendState blend_state;
    core::RasterizerDescriptor rasterization_descriptor;
    core::DepthStencilDescriptor depth_stencil_descriptor;
    core::StreamOutput stream_output;

    MaterialPSOCompilationContext(core::VertexAttributeSpecificationList const& va_list);
};

struct MaterialShaderDesc
{
    core::dx::d3d12::tasks::HLSLCompilationTask* p_vertex_shader_compilation_task;
    core::dx::d3d12::tasks::HLSLCompilationTask* p_hull_shader_compilation_task;
    core::dx::d3d12::tasks::HLSLCompilationTask* p_domain_shader_compilation_task;
    core::dx::d3d12::tasks::HLSLCompilationTask* p_geometry_shader_compilation_task;
    core::dx::d3d12::tasks::HLSLCompilationTask* p_pixel_shader_compilation_task;

    std::string material_parameters_uniform_buffer_name;
    std::string scene_parameters_uniform_buffer_name;
};

class MaterialAssemblyTask : public core::concurrency::SchedulableTask
{
public:
    constexpr static uint32_t c_reserved_srv_space_id_for_bindless_resources = 50;

    MaterialAssemblyTask(
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        MaterialPSOCompilationContext const& context,
        MaterialShaderDesc const& shaders
    );

    // AbstractTask protocol implementation
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    core::concurrency::TaskType type() const override { return core::concurrency::TaskType::cpu; }

    core::dx::d3d12::ConstantBufferReflection const& getMaterialParametersUniformBufferReflection() const { return m_material_parameters_cb_reflection; }
    core::dx::d3d12::ConstantBufferReflection const& getSceneParametersUniformBufferReflection() const { return m_scene_parameters_cb_reflection; }
    core::dx::dxcompilation::ShaderStage* getPixelShaderStage() const { return m_shader_function.getShaderStage(core::dx::dxcompilation::ShaderType::pixel); }

    void bindMaterialParameters(
        core::dx::d3d12::CommandList& target_command_list,
        core::dx::d3d12::ConstantBufferDataMapper& data_mapper
    );

    void bindSceneParameters(
        core::dx::d3d12::CommandList& target_command_list,
        core::dx::d3d12::ConstantBufferDataMapper& data_mapper
    );

private:
    core::dx::d3d12::BasicRenderingServices& m_basic_rendering_services;
    core::dx::dxcompilation::ShaderFunction m_shader_function;
    std::string m_material_parameters_ub_name;
    std::string m_scene_parameters_ub_name;
    core::dx::d3d12::ConstantBufferReflection m_material_parameters_cb_reflection;
    core::dx::d3d12::ConstantBufferReflection m_scene_parameters_cb_reflection;
    core::dx::d3d12::GraphicsPSODescriptor m_pso_descriptor;
    core::dx::d3d12::tasks::RootSignatureCompilationTask* m_root_signature_compilation_task = nullptr;
    core::dx::d3d12::tasks::GraphicsPSOCompilationTask* m_pso_compilation_task = nullptr;
};

// TODO: decouple shader function, descriptor allocation and PSO from Material
class Material : public core::NamedEntity<class_names::Material>
{
public:
    static const char* const c_material_parameters_uniform_buffer_name;

public:
    struct MetallicRoughness
    {
        core::math::Vector4f base_color_factor;
        float metallic_factor;
        float roughness_factor;
        Texture* p_base_color;
        Texture* p_metallic_roughness;
    };

public:
    Material(MaterialAssemblyTask& material_assembly_task);

    void setStringName(std::string const& entity_string_name);

    void setEmissiveFactor(glm::vec3 const& value) { m_emissive_factor = value; }
    void setAlphaMode(AlphaMode value) { m_alpha_mode = value; }
    void setAlphaCutoff(float value) { m_alpha_cutoff = value; }
    void setDoubleSided(bool value) { m_is_double_sided = value; }

    void setMetallicRoughness(MetallicRoughness const& value);
    void setNormalTexture(Texture* p_texture);
    void setOcclusionTexture(Texture* p_texture);
    void setEmissiveTexture(Texture* p_texture);

    void bindMaterialConstants(core::dx::d3d12::CommandList& target_command_list);


private:
    MaterialAssemblyTask& m_material_assembly;
    core::dx::d3d12::ConstantBufferDataMapper m_material_parameters_cb_data_mapper;

    std::unique_ptr<MaterialAssemblyTask> m_pso_compilation_task;
    
    core::math::Vector3f m_emissive_factor;
    AlphaMode m_alpha_mode;
    float m_alpha_cutoff;
    bool m_is_double_sided;
    std::vector<Material*> m_lod_materials;

    core::math::Vector4f m_base_color_factor;
    float m_metallic_factor;
    float m_roughness_factor;

    size_t m_normal_texture_binding_id;
    size_t m_occlusion_texture_binding_id;
    size_t m_emissive_texture_binding_id;

    size_t m_base_color_texture_binding_id;
    size_t m_metallic_roughness_texture_binding_id;
};



}

#endif