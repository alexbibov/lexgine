#ifndef LEXGINE_SCENEGRAPH_MATERIAL_H
#define LEXGINE_SCENEGRAPH_MATERIAL_H

#include <memory>
#include <vector>
#include <string>

#include <engine/core/math/vector_types.h>
#include <engine/core/dx/dxcompilation/shader_function.h>
#include <engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h>
#include <engine/core/dx/d3d12/constant_buffer_reflection.h>
#include <engine/core/dx/d3d12/constant_buffer_data_mapper.h>
#include <engine/core/dx/d3d12/pipeline_state.h>
#include <engine/core/dx/d3d12/caches/pso_blob_cache.h>
#include <engine/core/stream_output.h>
#include <engine/core/vertex_attributes.h>
#include <engine/core/misc/hash_value.h>
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
    core::dx::d3d12::caches::HLSLShaderHandle vertex_shader;
    core::dx::d3d12::caches::HLSLShaderHandle hull_shader;
    core::dx::d3d12::caches::HLSLShaderHandle domain_shader;
    core::dx::d3d12::caches::HLSLShaderHandle geometry_shader;
    core::dx::d3d12::caches::HLSLShaderHandle pixel_shader;

    std::string material_parameters_uniform_buffer_name;
    std::string scene_parameters_uniform_buffer_name;
};

class MaterialStaticState
{
public:
    constexpr static uint32_t c_reserved_srv_space_id_for_bindless_resources = 50;

    MaterialStaticState(
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        MaterialPSOCompilationContext const& context,
        MaterialShaderDesc const& shader_desc
    );

    void buildPipeline();     //! create PSO compilation contract in PSOBlobCache

    core::dx::d3d12::ConstantBufferReflection const& getMaterialParametersUniformBufferReflection() const { return m_material_parameters_cb_reflection; }
    core::dx::d3d12::ConstantBufferReflection const& getObjectParametersUniformBufferReflection() const { return m_object_parameters_cb_reflection; }
    core::dx::d3d12::ConstantBufferReflection const& getSceneParametersUniformBufferReflection() const { return m_scene_parameters_cb_reflection; }
    core::dx::dxcompilation::ShaderStage* getShaderStage(core::dx::dxcompilation::ShaderType shader_type) const
    {
        return m_shader_function->getShaderStage(shader_type);
    }

    void bindMaterialParameters(
        core::dx::d3d12::CommandList& target_command_list,
        core::dx::d3d12::ConstantBufferDataMapper& data_mapper
    ) const;

    void bindObjectParameters(
        core::dx::d3d12::CommandList& target_command_list,
        core::dx::d3d12::ConstantBufferDataMapper& data_mapper
    ) const;

    void bindSceneParameters(
        core::dx::d3d12::CommandList& target_command_list,
        core::dx::d3d12::ConstantBufferDataMapper& data_mapper
    ) const;

    core::dx::d3d12::GraphicsPSODescriptor const& pipelineDescriptor() const { return m_pso_descriptor; }

    bool operator==(MaterialStaticState const& other) const;

private:
    core::dx::d3d12::BasicRenderingServices& m_basic_rendering_services;
    std::unique_ptr<core::dx::dxcompilation::ShaderFunction> m_shader_function;
    std::string m_material_parameters_ub_name;
    std::string m_scene_parameters_ub_name;
    core::dx::d3d12::ConstantBufferReflection m_material_parameters_cb_reflection;
    core::dx::d3d12::ConstantBufferReflection m_object_parameters_cb_reflection;
    core::dx::d3d12::ConstantBufferReflection m_scene_parameters_cb_reflection;
    core::dx::d3d12::GraphicsPSODescriptor m_pso_descriptor;
    core::dx::d3d12::caches::RootSignatureHandle m_rs_handle { nullptr };
    core::dx::d3d12::caches::GraphicsPSOHandle m_pso_handle { nullptr };
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
    Material(MaterialStaticState const& material_static_state);

    void setStringName(std::string const& entity_string_name);

    void setEmissiveFactor(glm::vec3 const& value) { m_emissive_factor = value; }
    void setAlphaMode(AlphaMode value) { m_alpha_mode = value; }
    void setAlphaCutoff(float value) { m_alpha_cutoff = value; }
    void setDoubleSided(bool value) { m_is_double_sided = value; }

    void setMetallicRoughness(MetallicRoughness const& value);
    void setNormalTexture(Texture* p_texture);
    void setOcclusionTexture(Texture* p_texture);
    void setEmissiveTexture(Texture* p_texture);

    MaterialStaticState const& getStaticState() const { return m_material_static_state; }
    core::dx::d3d12::ConstantBufferDataMapper& getMaterialConstants() { return m_material_parameters_cb_data_mapper; }

private:
    MaterialStaticState const& m_material_static_state;
    core::dx::d3d12::ConstantBufferDataMapper m_material_parameters_cb_data_mapper;
    
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
