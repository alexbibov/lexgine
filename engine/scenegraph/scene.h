#ifndef LEXGINE_SCENEGRAPH_SCENE_H
#define LEXGINE_SCENEGRAPH_SCENE_H

#include <filesystem>
#include <future>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <tinygltf/tiny_gltf_v3.h>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/scenegraph/lexgine_scenegraph_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/misc/datetime.h"
#include "engine/core/misc/optional.h"
#include "engine/core/dx/d3d12/d3d12_tools.h"
#include "class_names.h"
#include "scene_mesh_memory.h"
#include "mesh.h"
#include "buffer_view.h"
#include "light.h"
#include "image.h"
#include "node.h"
#include "sampler.h"
#include "camera.h"

namespace lexgine::scenegraph
{

enum class SceneSource
{
    gltf,
    glb
};

class Scene : public core::NamedEntity<class_names::Scene>, public std::enable_shared_from_this<Scene>
{
public:
    static std::shared_ptr<Scene> loadScene(
        core::Globals& globals, 
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services, 
        std::filesystem::path const& path_to_scene, unsigned scene_id
    );
    static std::shared_ptr<Scene> loadScene(
        core::Globals& globals, 
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        std::filesystem::path const& path_to_scene, 
        std::string const& scene_name
    );

    SceneSource getSceneSource() const { return m_scene_source; }
    bool loadStatus() const;

private:
    static constexpr char const* c_khr_light_punctual_ext = "KHR_lights_punctual";
    static constexpr char const* c_ext_mesh_gpu_instancing = "EXT_mesh_gpu_instancing";

private:
    struct SceneMemory
    {
        std::unique_ptr<SceneMeshMemory> scene_memory_buffer;
        std::vector<SceneMemoryBufferHandle> m_scene_memory_handles;

        SceneMemoryBufferHandle getBuffer(size_t id) { return m_scene_memory_handles[id]; }
    };

    struct MaterialStaticStateCreateInfo
    {
        core::dx::d3d12::caches::HLSLShaderHandle vertex_shader;
        core::dx::d3d12::caches::HLSLShaderHandle hull_shader;
        core::dx::d3d12::caches::HLSLShaderHandle domain_shader;
        core::dx::d3d12::caches::HLSLShaderHandle geometry_shader;
        core::dx::d3d12::caches::HLSLShaderHandle pixel_shader;
        core::VertexAttributeSpecificationList vertex_data_format;
        size_t hash() const;
    };

    struct MaterialStaticStateCreateInfoHasher
    {
        size_t operator()(MaterialStaticStateCreateInfo const& value) const
        {
            return value.hash();
        }
    };

    struct MaterialStaticStateCreateInfoComparator
    {
        bool operator() (MaterialStaticStateCreateInfo const& lhs,
            MaterialStaticStateCreateInfo const& rhs) const
        {
            if (&lhs == &rhs) return true;

            bool test =
                lhs.vertex_shader == rhs.vertex_shader
                && lhs.hull_shader == rhs.hull_shader
                && lhs.domain_shader == rhs.domain_shader
                && lhs.geometry_shader == rhs.geometry_shader
                && lhs.pixel_shader == rhs.pixel_shader;
            if (!test) return false;

            if (lhs.vertex_data_format.size() != rhs.vertex_data_format.size())
            {
                return false;
            }

            for (size_t i = 0; i < lhs.vertex_data_format.size(); ++i)
            {
                auto const& lhs_va = lhs.vertex_data_format[i];
                auto const& rhs_va = rhs.vertex_data_format[i];

                if ((!lhs_va && rhs_va) || (lhs_va && !rhs_va))
                {
                    return false;
                }
                if (!lhs_va || !rhs_va)
                {
                    continue;
                }

                if (lhs_va->input_slot() != rhs_va->input_slot()
                    || lhs_va->offset() != rhs_va->offset()
                    || lhs_va->name_index() != rhs_va->name_index()
                    || lhs_va->instancingRate() != rhs_va->instancingRate()
                    || lhs_va->size() != rhs_va->size()
                    || lhs_va->capacity() != rhs_va->capacity()
                    || lhs_va->format<core::EngineApi::Direct3D12>() != rhs_va->format<core::EngineApi::Direct3D12>()
                    || lhs_va->name() != rhs_va->name())
                {
                    return false;
                }
            }

            return true;
        }
    };

    using MaterialStaticStateCreateInfoSet =
        std::unordered_set<MaterialStaticStateCreateInfo, MaterialStaticStateCreateInfoHasher, MaterialStaticStateCreateInfoComparator>;

    struct MaterialAttachment
    {
        MaterialStaticStateCreateInfoSet::const_iterator create_info_it;
        std::vector<size_t> target_submesh_ids;
    };

private:
    Scene(
        core::Globals& globals, 
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        std::filesystem::path const& path_to_scene,
        unsigned scene_id
    );
    Scene(
        core::Globals& globals, 
        core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
        std::filesystem::path const& path_to_scene,
        std::string const& scene_name
    );

    std::optional<tinygltf3::Model> readGltfModel(std::filesystem::path const& path);
    bool readScene(tg3_model const& model, unsigned scene_index);

    bool loadLights(
        tg3_model const& model,
        std::unordered_map<int, int>& light_ids
    );
    bool loadTextures(
        tg3_model const& model,
        std::unordered_map<int, int>& texture_ids,
        std::unordered_map<int, int>& sampler_ids
    );
    bool loadMeshes(
        tg3_model const& model,
        std::unordered_map<int, int>& mesh_ids,
        std::unordered_map<int, int> const& buffer_ids
    );
    bool loadCameras(
        tg3_model const& model,
        std::unordered_map<int, int>& camera_ids
    );
    bool loadAnimations(
        tg3_model const& model,
        std::unordered_map<int, int>& animation_ids
    );

    MaterialStaticStateCreateInfoSet::const_iterator
        registerMaterialStaticState(
            tg3_material const& gltf_material,
            const lexgine::core::VertexAttributeSpecificationList& vertex_attributes
        );
    /*size_t registerMaterial(
        tg3_material const& gltf_material,
        const lexgine::core::VertexAttributeSpecificationList& vertex_attributes
    );*/
   

private:
    core::Globals& m_globals;
    core::dx::d3d12::BasicRenderingServices& m_basic_rendering_services;
    core::GlobalSettings& m_global_settings;
    core::misc::DateTime const m_timestamp;
    std::filesystem::path m_scene_path;
    SceneSource m_scene_source;
    int m_scene_index{ -1 };
    bool m_scene_source_parse_status{ false };
    std::unordered_map<std::string, bool> m_enabled_extensions = { {c_khr_light_punctual_ext, false} };
    
    std::vector<Node> m_scene_nodes;
    std::vector<Light> m_lights;
    std::vector<Texture> m_textures;
    std::vector<Sampler> m_samplers;
    std::vector<Material> m_materials;
    std::vector<Camera> m_cameras;
    std::vector<Mesh> m_scene_meshes;
    std::vector<BufferView> m_memory_views;

    SceneMemory m_scene_memory;
    MaterialStaticStateCreateInfoSet m_material_static_state_create_infos;
    std::unordered_map<size_t, MaterialAttachment> m_material_attachements;
};

}

#endif
