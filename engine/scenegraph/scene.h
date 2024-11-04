#ifndef LEXGINE_SCENEGRAPH_SCENE_H
#define LEXGINE_SCENEGRAPH_SCENE_H

#include <filesystem>
#include <unordered_map>

#include <engine/core/lexgine_core_fwd.h>
#include <engine/core/entity.h>
#include <engine/core/misc/datetime.h>
#include <engine/core/misc/optional.h>
#include "class_names.h"
#include "scene_mesh_memory.h"
#include "buffer_view.h"
#include "light.h"
#include "image.h"
#include "sampler.h"

namespace lexgine::scenegraph
{

struct SceneMeshDataDesc
{
    MeshBufferHandle handle;
    std::string name;
};

class Scene : public core::NamedEntity<class_names::Scene>
{
public:
    static std::shared_ptr<Scene> loadScene(core::Globals& globals, std::filesystem::path const& path_to_scene, int scene_index = -1);

private:
    static constexpr char const* c_khr_light_punctual_ext = "KHR_lights_punctual";
    static constexpr char const* c_ext_mesh_gpu_instancing = "EXT_mesh_gpu_instancing";


private:
    bool loadLights(tinygltf::Model& model);
    bool loadTextures(tinygltf::Model& model, core::misc::Optional<core::misc::DateTime> const& timestamp, conversion::ImageLoaderPool const& image_loader_pool);
    bool loadMaterials(tinygltf::Model& model);
    bool loadMeshes(tinygltf::Model& model);
    bool loadCameras(tinygltf::Model& model);
    bool loadNodes(tinygltf::Model& model);
    bool loadAnimations(tinygltf::Model& model);

private:
    Scene() = default;

private:
    std::unordered_map<std::string, bool> m_enabled_extensions = { {c_khr_light_punctual_ext, false} };
    std::filesystem::path m_scene_path;
    std::vector<Light> m_lights;
    std::vector<Image> m_images;
    std::vector<Sampler> m_samplers;

    
    std::unique_ptr<SceneMeshMemory> m_scene_memory;
    std::vector<SceneMeshDataDesc> m_scene_mesh_datas;
    std::vector<BufferView> m_memory_views;

};

}

#endif
