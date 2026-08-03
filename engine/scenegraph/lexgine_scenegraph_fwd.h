#ifndef LEXGINE_SCENEGRAPH_FWD_H
#define LEXGINE_SCENEGRAPH_FWD_H

#include <cstdint>
#include <limits>

namespace lexgine::scenegraph {

class Image;
class Material;
class Scene;
class SceneMeshMemory;
class BufferView;
class Mesh;
class Light;
class Camera;
struct SceneUniformBuffer;
struct SceneMemory;

static constexpr uint32_t c_scene_resource_invalid_id = std::numeric_limits<uint32_t>::max();

}

#endif