
#include <tinygltf/tiny_gltf.h>
#include "scene.h"

namespace lexgine::scenegraph
{

namespace
{



}

std::shared_ptr<Scene> Scene::loadScene(std::filesystem::path const& path_to_scene, int scene_index /* = -1 */)
{
    std::string error_buffer{};
    std::string warning_buffer{};

    tinygltf::TinyGLTF gltf_loader{};
    std::string gltf_path_to_file = path_to_scene.string();

    tinygltf::Model gltf_model{};

    bool result = gltf_loader.LoadASCIIFromFile(&gltf_model, &error_buffer, &warning_buffer, gltf_path_to_file, tinygltf::SectionCheck::REQUIRE_VERSION);
    Scene rv{};


    if (!error_buffer.empty())
    {
        LEXGINE_LOG_ERROR(rv, "Error loading gltf file '" + gltf_path_to_file + "': " + error_buffer);
    }

    if (!warning_buffer.empty())
    {
        core::misc::Log::retrieve()->out("Warning loading gltf file '" + gltf_path_to_file + "': " + warning_buffer, core::misc::LogMessageType::exclamation);
    }


    if (!result) {
        LEXGINE_LOG_ERROR(rv, "Unable to load gltf file '" + gltf_path_to_file + "'");
        return nullptr;
    }


    rv.setStringName("gltf_scene_" + path_to_scene.stem().string());

    // Check used extensions
    for (auto& ext : gltf_model.extensionsUsed)
    {
        if (auto it = rv.m_enabled_extensions.find(ext); it == rv.m_enabled_extensions.end())
        {

        }
    }
}


}
