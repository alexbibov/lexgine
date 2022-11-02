#ifndef LEXGINE_SCENEGRAPH_SCENE_H
#define LEXGINE_SCENEGRAPH_SCENE_H

#include <filesystem>
#include <unordered_map>

#include <engine/core/entity.h>
#include "class_names.h"

namespace lexgine::scenegraph
{

class Scene : public core::NamedEntity<class_names::Scene>
{
public:
    static std::shared_ptr<Scene> loadScene(std::filesystem::path const& path_to_scene, int scene_index = -1);

private:
    static constexpr char const* c_khr_light_punctual_ext = "KHR_lights_punctual";

private:
    std::unordered_map<std::string, bool> m_enabled_extensions = { {c_khr_light_punctual_ext, false} };
private:
    Scene();

};

}

#endif
