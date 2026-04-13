#ifndef LEXGINE_SCENEGRAPH_SCENE_H
#define LEXGINE_SCENEGRAPH_SCENE_H

#include <filesystem>
#include <future>
#include <optional>
#include <unordered_map>

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
    bool loadMaterial(
        tg3_material const& gltf_material,
        const lexgine::core::VertexAttributeSpecificationList& vertex_attributes
    );
    void scheduleMaterialConstruction();
    bool loadCameras(
        tg3_model const& model,
        std::unordered_map<int, int>& camera_ids
    );
    bool loadAnimations(
        tg3_model const& model,
        std::unordered_map<int, int>& animation_ids
    );

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

    SceneMemory m_scene_memory;

    std::vector<Mesh> m_scene_meshes;
    std::vector<BufferView> m_memory_views;
    std::vector<std::unique_ptr<MaterialAssemblyTask>> m_material_construction_tasks;
    std::unique_ptr<core::concurrency::TaskGraph> m_material_construction_task_graph;
    std::unique_ptr<core::concurrency::TaskSink> m_material_construction_task_sink;
};

}

#endif
