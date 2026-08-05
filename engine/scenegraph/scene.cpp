
#include <algorithm>
#include <cctype>
#include <cmath>
#include <future>
#include <numeric>
#include <thread>

#include <glm/gtc/constants.hpp>

#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#include <tinygltf/tiny_gltf_v3.h>
#undef TINYGLTF3_IMPLEMENTATION
#undef TINYGLTF3_ENABLE_FS

#include <engine/core/globals.h>
#include <engine/core/misc/misc.h>
#include <engine/core/misc/hashes/xxhash64.h>
#include <engine/core/dx/d3d12/dx_resource_factory.h>
#include <engine/core/dx/d3d12/basic_rendering_services.h>
#include <engine/core/concurrency/task_graph.h>
#include <engine/core/concurrency/task_sink.h>
#include <engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h>
#include <engine/conversion/image_loader_pool.h>
#include <engine/conversion/texture_converter.h>
#include "scene.h"

namespace lexgine::scenegraph
{

namespace
{

core::misc::DateTime fetchTimestamp(std::filesystem::path const& gltf_path_to_file)
{
    // Fetch the time stamp to be used by the relevant components of the scene: this shall be the update time of the source
    // file containing the scene. Otherwise, build time of this translation unit will be used as the the timestamp
    auto gltf_timestamp = core::misc::getFileLastUpdatedTimeStamp(gltf_path_to_file.string());
    return gltf_timestamp.isValid() ? *gltf_timestamp : core::misc::DateTime::buildTime();
}

int getSceneIndexFromName(tg3_model const& model, std::string const& scene_name)
{
    for (uint32_t i = 0; i < model.scenes_count; ++i) {
        if (tg3_str_equals_cstr(model.scenes[i].name, scene_name.c_str())) {
            return static_cast<int>(i);
        }
    }
    return -1;
}


template<typename T>
T gltfCast(int gltf_value)
{
    return T{};
}


template<>
lexgine::core::misc::DataFormat gltfCast<lexgine::core::misc::DataFormat>(int gltf_component_type)
{
    switch (gltf_component_type) {
    case TG3_COMPONENT_TYPE_BYTE:
        return lexgine::core::misc::DataFormat::int8;

    case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
        return lexgine::core::misc::DataFormat::uint8;

    case TG3_COMPONENT_TYPE_SHORT:
        return lexgine::core::misc::DataFormat::int16;

    case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
        return lexgine::core::misc::DataFormat::uint16;

    case TG3_COMPONENT_TYPE_INT:
        return lexgine::core::misc::DataFormat::int32;

    case TG3_COMPONENT_TYPE_UNSIGNED_INT:
        return lexgine::core::misc::DataFormat::uint32;

    case TG3_COMPONENT_TYPE_FLOAT:
        return lexgine::core::misc::DataFormat::float32;

    case TG3_COMPONENT_TYPE_DOUBLE:
        return lexgine::core::misc::DataFormat::float64;

    default:
        return lexgine::core::misc::DataFormat::unknown;
    }
}

template<>
lexgine::scenegraph::MinificationFilter gltfCast<lexgine::scenegraph::MinificationFilter>(int gltf_minification_filter)
{
    switch (gltf_minification_filter)
    {
    case TG3_TEXTURE_FILTER_NEAREST:
        return MinificationFilter::nearest;

    case TG3_TEXTURE_FILTER_LINEAR:
        return MinificationFilter::linear;

    case TG3_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        return MinificationFilter::nearest_mipmap_nearest;

    case TG3_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        return MinificationFilter::linear_mipmap_nearest;

    case TG3_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        return MinificationFilter::nearest_mipmap_linear;

    case TG3_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        return MinificationFilter::linear_mipmap_linear;

    default:
        return MinificationFilter::linear_mipmap_linear;
    }
}

template <>
lexgine::scenegraph::MagnificationFilter gltfCast<lexgine::scenegraph::MagnificationFilter>(int gltf_magnification_filter)
{
    switch (gltf_magnification_filter) {
    case TG3_TEXTURE_FILTER_NEAREST:
        return MagnificationFilter::nearest;

    case TG3_TEXTURE_FILTER_LINEAR:
        return MagnificationFilter::linear;

    default:
        return MagnificationFilter::linear;
    }
}

template<>
lexgine::scenegraph::WrapMode gltfCast<lexgine::scenegraph::WrapMode>(int gltf_wrapping_mode)
{
    switch (gltf_wrapping_mode)
    {
    case TG3_TEXTURE_WRAP_REPEAT:
        return WrapMode::repeat;

    case TG3_TEXTURE_WRAP_MIRRORED_REPEAT:
        return WrapMode::mirrored_repeat;

    case TG3_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return WrapMode::clamp_to_edge;

    default:
        return WrapMode::repeat;
    }
}

std::pair<std::string, unsigned> extractNameAndIndexFromAttributeName(std::string const& attribute_name)
{
    size_t name_length = attribute_name.find_last_not_of("0123456789") + 1;
    bool has_index = name_length < attribute_name.size();
    unsigned index = has_index ? static_cast<unsigned>(std::stoul(attribute_name.substr(name_length))) : 0;

    // glTF spells indexed attributes as "<NAME>_<index>" (e.g. "TEXCOORD_0"); drop the
    // trailing '_' separator so the semantic name matches the shader input signature.
    size_t name_end = name_length;
    if (has_index && name_end > 0 && attribute_name[name_end - 1] == '_')
        --name_end;

    std::string name = attribute_name.substr(0, name_end);
    std::vector<char> uppercase_name; uppercase_name.resize(name.length());
    std::transform(name.begin(), name.end(), uppercase_name.begin(), [](char e) { return static_cast<char>(std::toupper(e)); });
    name = std::string{ uppercase_name.data(), uppercase_name.size() };
    return { name, index };
}

}  // namespace

#pragma region Scene
std::shared_ptr<Scene> Scene::loadScene(
    core::Globals& globals,
    core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
    std::filesystem::path const& path_to_scene,
    unsigned scene_id
)
{
    auto rv = std::shared_ptr<Scene>{ new Scene(globals, basic_rendering_services, path_to_scene, scene_id) };
    return rv;
}

std::shared_ptr<Scene> Scene::loadScene(
    core::Globals& globals, 
    core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
    std::filesystem::path const& path_to_scene,
    std::string const& scene_name
)
{
    auto rv = std::shared_ptr<Scene>{ new Scene(globals, basic_rendering_services, path_to_scene, scene_name) };
    return rv;
}

#pragma region SceneNode
Scene::Node::Node(Scene* owner, uint32_t self_id)
    : m_owner{ owner }
    , m_self_id{ self_id }
    , m_parent_to_local_transform{ 1.f }
    , m_local_to_parent_transform{ 1.f }
    , m_world_to_local_transform{ 1.f }
    , m_local_to_world_transform{ 1.f }
{

}

void Scene::Node::addChild(uint32_t child_id)
{
    Node& child = m_owner->m_scene_nodes[child_id];
    if (child.m_parent_id != c_scene_resource_invalid_id)
    {
        m_owner->m_scene_nodes[child.m_parent_id].removeChild(child_id);
    }

    m_children.push_back(child_id);
    child.m_parent_id = m_self_id;

    if (!child.m_is_dirty)
    {
        child.invalidateSubtree();
    }
}

void Scene::Node::removeChild(uint32_t child_id)
{
    auto it = std::find(m_children.begin(), m_children.end(), child_id);
    if (it != m_children.end())
    {
        m_children.erase(it);
        Node& child = m_owner->m_scene_nodes[child_id];
        child.m_parent_id = c_scene_resource_invalid_id;
        child.invalidateSubtree();
    }
}

core::math::Matrix4f const& Scene::Node::parentToLocalTransform() const
{
    updateTransforms();
    return m_parent_to_local_transform;
}

core::math::Matrix4f const& Scene::Node::localToParentTransform() const
{
    updateTransforms();
    return m_local_to_parent_transform;
}

core::math::Matrix4f const& Scene::Node::worldToLocalTransform() const
{
    updateTransforms();
    return m_world_to_local_transform;
}

core::math::Matrix4f const& Scene::Node::localToWorldTransform() const
{
    updateTransforms();
    return m_local_to_world_transform;
}

core::math::Vector4f const& Scene::Node::worldPositionH() const
{
    updateTransforms();
    return m_local_to_world_transform[3];
}

core::math::Vector3f Scene::Node::worldPosition() const
{
    auto const& position_h = worldPositionH();
    return core::math::Vector3f{ position_h.x, position_h.y, position_h.z };
}

void Scene::Node::setTranslation(core::math::Vector3f const& translation_vector)
{
    m_translation = translation_vector;
    recomputeLocalTransform();
    invalidateSubtree();
}

void Scene::Node::setRotation(core::math::Vector3f const& rotation_axis, float angle)
{
    float const axis_length = std::sqrt(
        rotation_axis.x * rotation_axis.x +
        rotation_axis.y * rotation_axis.y +
        rotation_axis.z * rotation_axis.z);

    if (axis_length < 1e-6f)
    {
        m_rotation = core::math::Matrix4f{ 1.f };
    }
    else
    {
        float const nx = rotation_axis.x / axis_length;
        float const ny = rotation_axis.y / axis_length;
        float const nz = rotation_axis.z / axis_length;

        core::math::Matrix4f K{
            core::math::Vector4f{ 0.f, nz, -ny, 0.f },
            core::math::Vector4f{ -nz, 0.f, nx, 0.f },
            core::math::Vector4f{ ny, -nx, 0.f, 0.f },
            core::math::Vector4f{ 0.f, 0.f, 0.f, 1.f }
        };
        m_rotation = core::math::Matrix4f{ 1.f } + std::sin(angle) * K + (1.f - std::cos(angle)) * (K * K);
    }

    recomputeLocalTransform();
    invalidateSubtree();
}

void Scene::Node::setScale(core::math::Vector3f const& scaling_vector)
{
    m_scale = scaling_vector;
    recomputeLocalTransform();
    invalidateSubtree();
}

void Scene::Node::recomputeLocalTransform()
{
    core::math::Matrix4f scale_transform{
        core::math::Vector4f{ m_scale.x, 0.f, 0.f, 0.f },
        core::math::Vector4f{ 0.f, m_scale.y, 0.f, 0.f },
        core::math::Vector4f{ 0.f, 0.f, m_scale.z, 0.f },
        core::math::Vector4f{ 0.f, 0.f, 0.f, 1.f }
    };

    core::math::Matrix4f translation_transform{
        core::math::Vector4f{ 1.f, 0.f, 0.f, 0.f },
        core::math::Vector4f{ 0.f, 1.f, 0.f, 0.f },
        core::math::Vector4f{ 0.f, 0.f, 1.f, 0.f },
        core::math::Vector4f{ m_translation.x, m_translation.y, m_translation.z, 1.f }
    };

    m_local_to_parent_transform = translation_transform * m_rotation * scale_transform;
    m_parent_to_local_transform = glm::inverse(m_local_to_parent_transform);
}

void Scene::Node::setLight(uint32_t light_id)
{
    m_light_id = light_id;
    m_camera_id = c_scene_resource_invalid_id;
    m_mesh_id = c_scene_resource_invalid_id;
}

void Scene::Node::setCamera(uint32_t camera_id)
{
    m_camera_id = camera_id;
    m_light_id = c_scene_resource_invalid_id;
    m_mesh_id = c_scene_resource_invalid_id;
}

void Scene::Node::setMesh(uint32_t mesh_id)
{
    m_mesh_id = mesh_id;
    m_camera_id = c_scene_resource_invalid_id;
    m_light_id = c_scene_resource_invalid_id;
}

void Scene::Node::invalidateSubtree()
{
    m_is_dirty = true;
    for (uint32_t child_id : m_children)
    {
        m_owner->m_scene_nodes[child_id].invalidateSubtree();
    }
}

void Scene::Node::updateTransforms() const
{
    if (!m_is_dirty)
    {
        return;
    }

    if (m_parent_id == c_scene_resource_invalid_id)
    {
        m_world_to_local_transform = m_parent_to_local_transform;
        m_local_to_world_transform = m_local_to_parent_transform;
    }
    else
    {
        Node const& parent = m_owner->m_scene_nodes[m_parent_id];
        parent.updateTransforms();

        m_world_to_local_transform = m_parent_to_local_transform * parent.m_world_to_local_transform;
        m_local_to_world_transform = parent.m_local_to_world_transform * m_local_to_parent_transform;
    }

    m_is_dirty = false;
}
#pragma endregion SceneNode

void Scene::removeNode(uint32_t node_id)
{
    Node& node = m_scene_nodes[node_id];

    uint32_t const parent_id = node.getParentId();
    if (parent_id != c_scene_resource_invalid_id)
    {
        m_scene_nodes[parent_id].removeChild(node_id);
    }

    // Orphan children. Copy the id list first since removeChild mutates the child vector.
    std::vector<uint32_t> const children = node.children();
    for (uint32_t child_id : children)
    {
        node.removeChild(child_id);
    }
}

void Scene::updateGeometryTransforms()
{
    uint32_t gpu_instance_section_carret = 0;
    for (Draw& d : m_draws)
    {
        d.gpu_instancing_section_start_index = gpu_instance_section_carret;
        for (uint32_t cpu_instance_id : d.instance_indices)
        {
            PerInstanceCpuData const& instance_cpu_data = m_instances_cpu_data[cpu_instance_id];
            PerInstanceGpuData& instance_gpu_data = m_instances_gpu_data[gpu_instance_section_carret];
            Node const& n = m_scene_nodes[instance_cpu_data.owning_node_id];
            instance_gpu_data.transform = n.localToWorldTransform();
            ++gpu_instance_section_carret;
        }
    }
}

uint32_t Scene::getCameraId(core::misc::HashedString const& camera_name) const
{
    auto it = m_camera_names_lut.find(camera_name);
    return it != m_camera_names_lut.end() ? static_cast<uint32_t>(it->second) : c_scene_resource_invalid_id;
}

uint32_t Scene::getLightId(core::misc::HashedString const& light_name) const
{
    auto it = m_light_names_lut.find(light_name);
    return it != m_light_names_lut.end() ? static_cast<uint32_t>(it->second) : c_scene_resource_invalid_id;
}

void Scene::setCurrentCamera(uint32_t camear_id)
{
    m_current_camera_node_id = camear_id;
    Node& current_camera_node = m_scene_nodes[m_current_camera_node_id];
    Camera& current_camera = m_cameras[current_camera_node.getCamera()];
    m_current_camera_position = current_camera_node.worldPosition();
    m_scene_parameters_data_mapper->addOrUpdateDataBinding("view", current_camera.getViewMatrix());
    m_scene_parameters_data_mapper->addOrUpdateDataBinding("projection", current_camera.getProjectionMatrix());
    m_scene_parameters_data_mapper->addOrUpdateDataBinding("view_projection", current_camera.getViewProjectionMatrix());
    m_scene_parameters_data_mapper->addOrUpdateDataBinding("inv_projection", current_camera.getInverseProjectionMatrix());
    m_scene_parameters_data_mapper->addOrUpdateDataBinding("inv_view_projection", current_camera.getInverseViewProjectionMatrix());
    m_scene_parameters_data_mapper->addOrUpdateDataBinding("camera_position", m_current_camera_position);
}

Scene::Scene(
    core::Globals& globals,
    core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
    std::filesystem::path const& path_to_scene, 
    unsigned scene_id
)
    : m_globals{ globals }
    , m_basic_rendering_services{ basic_rendering_services }
    , m_global_settings{ *globals.get<core::GlobalSettings>() }
    , m_timestamp{ fetchTimestamp(path_to_scene) }
    , m_scene_path{ path_to_scene }
    , m_scene_index{ static_cast<int>(scene_id) }
{
    auto gltf_model_opt = readGltfModel(path_to_scene);
    if (!gltf_model_opt)
    {
        return;
    }
    tg3_model const& gltf_model = *gltf_model_opt->get();
    setStringName(std::string(gltf_model.scenes[scene_id].name.data, gltf_model.scenes[scene_id].name.len));
    m_scene_source_parse_status = readScene(gltf_model, scene_id);
}

Scene::Scene(
    core::Globals& globals, 
    core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
    std::filesystem::path const& path_to_scene, 
    std::string const& scene_name
)
    : m_globals{ globals }
    , m_global_settings { *globals.get<core::GlobalSettings>() }
    , m_basic_rendering_services{ basic_rendering_services }
    , m_timestamp{ fetchTimestamp(path_to_scene) }
    , m_scene_path{ path_to_scene }
{
    auto gltf_model_opt = readGltfModel(path_to_scene);
    if (!gltf_model_opt)
    {
        return;
    }
    tg3_model const& gltf_model = *gltf_model_opt->get();

    m_scene_index = getSceneIndexFromName(gltf_model, scene_name);
    setStringName(scene_name);

    if (m_scene_index == -1)
    {
        LEXGINE_LOG_ERROR(this, "Unable to load gltf file '" + path_to_scene.string() + "': scene with name '" + scene_name + "' not found");
        m_scene_source_parse_status = false;
        return;
    }

    m_scene_source_parse_status = readScene(gltf_model, m_scene_index);
}

bool Scene::loadStatus() const
{
    if (!m_scene_source_parse_status) return false;
    conversion::TextureConverter& texture_converter = *m_globals.get<conversion::TextureConverter>();
    return 
        texture_converter.isTextureConversionCompleted()
        && texture_converter.isTextureUploadCompleted();
}

#pragma region Scene_MaterialStaticStateCreateInfo
size_t Scene::MaterialStaticStateCreateInfo::hash() const
{
    core::misc::hashes::XXHash64 h{};
    h.create(&vertex_shader.p_internal, sizeof(vertex_shader.p_internal));
    h.combine(&hull_shader.p_internal, sizeof(hull_shader.p_internal));
    h.combine(&domain_shader.p_internal, sizeof(domain_shader.p_internal));
    h.combine(&geometry_shader.p_internal, sizeof(geometry_shader.p_internal));
    h.combine(&pixel_shader.p_internal, sizeof(pixel_shader.p_internal));
    for (auto const& va : vertex_data_format)
    {
        size_t va_hash = va->hash<core::EngineApi::Direct3D12>();
        h.combine(&va_hash, sizeof(va_hash));
    }
    h.finalize();
    return static_cast<size_t>(h.fold());
}
#pragma endregion

[[nodiscard]]
std::optional<tinygltf3::Model> Scene::readGltfModel(std::filesystem::path const& path)
{
    std::string gltf_path_to_file = path.string();
    {
        std::string extension = path.extension().string();
        std::vector<char> buf(extension.size());
        std::transform(extension.begin(), extension.end(), buf.begin(), [](char c) { return static_cast<char>(std::tolower(c)); });
        extension = std::string{ buf.begin(), buf.end() };
        if (extension == ".gltf")
        {
            m_scene_source = SceneSource::gltf;
        }
        else if (extension == ".bin")
        {
            m_scene_source = SceneSource::glb;
        }
        else
        {
            LEXGINE_LOG_ERROR(this, std::format("Unable to load scene '{}': scene file has unsupported format extension '{}'", gltf_path_to_file, extension));
            return std::nullopt;
        }
    }

    tinygltf3::Model gltf_model;
    tinygltf3::ErrorStack errors;
    tg3_parse_options options;
    tg3_parse_options_init(&options);

    tg3_error_code result = tg3_parse_file(
        gltf_model.get(), errors.get(),
        gltf_path_to_file.c_str(), static_cast<uint32_t>(gltf_path_to_file.size()),
        &options);

    for (uint32_t i = 0; i < errors.count(); ++i)
    {
        tg3_error_entry const* e = errors.entry(i);
        if (e->severity == TG3_SEVERITY_ERROR)
        {
            LEXGINE_LOG_ERROR(this, e->message);
        }
        else
        {
            logger().out(e->message, core::misc::LogMessageType::exclamation);
        }
    }

    if (result != TG3_OK || errors.has_error())
        return std::nullopt;

    return std::move(gltf_model);
}

bool Scene::readScene(tg3_model const& model, unsigned scene_index)
{
    std::string gltf_path_to_file = m_scene_path.string();

    // Check used extensions
    for (uint32_t ei = 0; ei < model.extensions_used_count; ++ei)
    {
        std::string ext(model.extensions_used[ei].data, model.extensions_used[ei].len);
        auto it = m_enabled_extensions.find(ext);
        if (it == m_enabled_extensions.end())
        {
            bool required = false;
            for (uint32_t ri = 0; ri < model.extensions_required_count; ++ri)
            {
                if (tg3_str_equals_cstr(model.extensions_required[ri], ext.c_str()))
                {
                    required = true;
                    break;
                }
            }
            if (required)
            {
                LEXGINE_LOG_ERROR(this, "Unable to load gltf file '" + gltf_path_to_file + "': required extension " + ext + " is not supported");
                return false;
            }
            core::misc::Log::retrieve()->out("gltf file '" + gltf_path_to_file + "' contains unsupported extension " + ext, core::misc::LogMessageType::exclamation);
        }
        else
        {
            it->second = true;
        }
    }

    GltfToSceneIndexMap index_map;
    {
        tg3_scene const& scene = model.scenes[scene_index];

        {
            // Create default camera and light nodes

            m_cameras.push_back(Camera{ "default_camera" });
            uint32_t const default_camera_id = static_cast<uint32_t>(m_cameras.size() - 1);
            uint32_t const default_camera_node_id = static_cast<uint32_t>(m_scene_nodes.size());
            m_scene_nodes.emplace_back(this, default_camera_node_id);
            m_scene_nodes.back().setStringName("default_camera_node");
            m_scene_nodes.back().setCamera(default_camera_id);

            m_lights.push_back(Light{ LightType::directional });
            Light& default_light = m_lights.back();
            default_light.setDirection({ 0, -1, 0 });
            default_light.setIntensity(100.f);
            default_light.setStringName("default_light");
            uint32_t const default_light_id = static_cast<uint32_t>(m_lights.size() - 1);
            uint32_t const default_light_node_id = static_cast<uint32_t>(m_scene_nodes.size());
            m_scene_nodes.emplace_back(this, default_light_node_id);
            m_scene_nodes.back().setLight(default_light_id);
        }

        m_total_submesh_count = 0;
        for (uint32_t ni = 0; ni < scene.nodes_count; ++ni)
        {
            int node_id = scene.nodes[ni];
            tg3_node const& node = model.nodes[node_id];
            readSceneNode(model, node, index_map);
        }
    }


    // Prepare scene memory
    {
        uint64_t scene_memory_size = std::accumulate(index_map.buffer_ids.cbegin(), index_map.buffer_ids.cend(), 0ui64,
            [&model](uint64_t acc, std::pair<int, int> const& e) {
                return acc + model.buffers[e.first].data.count;
            });

        m_scene_memory.scene_memory_buffer.reset(new SceneMeshMemory{ m_globals, scene_memory_size });

        for (auto& [buffer_id, buffer_id_in_scene] : index_map.buffer_ids)
        {
            buffer_id_in_scene = m_scene_memory.m_scene_memory_handles.size();
            tg3_buffer const& buffer = model.buffers[buffer_id];
            m_scene_memory.m_scene_memory_handles.push_back(
                m_scene_memory.scene_memory_buffer->addData(
                    buffer.data.data,
                    buffer.data.count
                )
            );
        }
        m_scene_memory.scene_memory_buffer->uploadAllData();    // Upload all remaining scheduled data as soon as possible (some data may have already been uploaded depending on the size of the scene and the size of staging buffer)
    }

    bool load_result = true;
    if (!loadLights(
        model,
        index_map
    ))
    {
        LEXGINE_LOG_ERROR(this, "Unable to load lights when reading scene source \"" + m_scene_path.string() + "\"");
        load_result = false;
    }
    if (!loadTextures(
        model,
        index_map
    ))
    {
        LEXGINE_LOG_ERROR(this, "Unable to load textures when reading scene source \"" + m_scene_path.string() + "\"");
        load_result = false;
    }
    if (!loadMeshes(
        model,
        index_map
    ))
    {
        LEXGINE_LOG_ERROR(this, "Unable to load meshes when reading scene source \"" + m_scene_path.string() + "\"");
        load_result = false;
    }
    if (!loadCameras(
        model,
        index_map
    ))
    {
        LEXGINE_LOG_ERROR(this, "Unable to load cameras when reading scene source \"" + m_scene_path.string() + "\"");
        load_result = false;
    }

    {
        // Schedule shaders compilation
        auto* p_hlsl_shader_blob_cache = m_globals.get<core::dx::d3d12::caches::HLSLShaderBlobCache>();
        p_hlsl_shader_blob_cache->createShaderBlobs();
        p_hlsl_shader_blob_cache->waitTillReady();
    }

    {
        // Create materials 

        for (auto& [gltf_source_material_id, material_attachment] : m_material_attachements)
        {
            MaterialShaderDesc shader_desc{};
            shader_desc.vertex_shader = material_attachment.create_info_it->vertex_shader;
            shader_desc.hull_shader = material_attachment.create_info_it->hull_shader;
            shader_desc.domain_shader = material_attachment.create_info_it->domain_shader;
            shader_desc.geometry_shader = material_attachment.create_info_it->geometry_shader;
            shader_desc.pixel_shader = material_attachment.create_info_it->pixel_shader;

            shader_desc.material_parameters_uniform_buffer_name = "material_data";
            shader_desc.scene_parameters_uniform_buffer_name = "environment_data";

            MaterialPSOCompilationContext pso_context{ material_attachment.create_info_it->vertex_data_format };

            MaterialStaticState mss{ m_basic_rendering_services, pso_context, shader_desc };
            mss.buildPipeline();
            auto [it, res] = m_material_static_states.emplace(std::move(mss));
            assert(res);
            material_attachment.material_static_state_it = it;

            uint32_t new_material_id = static_cast<uint32_t>(m_materials.size());
            index_map.material_ids[gltf_source_material_id] = static_cast<int>(new_material_id);
            m_materials.emplace_back(*it);
            Material& material = m_materials.back();

            tg3_material const& source_material = model.materials[gltf_source_material_id];
            material.setStringName(std::string{ source_material.name.data, source_material.name.len });
            material.setEmissiveFactor(
                lexgine::core::math::Vector3f{
                    source_material.emissive_factor[0],
                    source_material.emissive_factor[1],
                    source_material.emissive_factor[2]
                }
            );
            //material.setAlphaMode()
            material.setAlphaCutoff(source_material.alpha_cutoff);
            material.setDoubleSided(source_material.double_sided != 0);
            {
                // Metallic-roughness
                Material::MetallicRoughness mr{};
                mr.base_color_factor = lexgine::core::math::Vector4f{
                    source_material.pbr_metallic_roughness.base_color_factor[0],
                    source_material.pbr_metallic_roughness.base_color_factor[1],
                    source_material.pbr_metallic_roughness.base_color_factor[2],
                    source_material.pbr_metallic_roughness.base_color_factor[3]
                };
                mr.metallic_factor = static_cast<float>(source_material.pbr_metallic_roughness.metallic_factor);
                mr.roughness_factor = static_cast<float>(source_material.pbr_metallic_roughness.roughness_factor);
                mr.p_base_color = source_material.pbr_metallic_roughness.base_color_texture.index >= 0
                    ? &m_textures[index_map.texture_ids.at(source_material.pbr_metallic_roughness.base_color_texture.index)]
                    : nullptr;
                mr.p_metallic_roughness = source_material.pbr_metallic_roughness.metallic_roughness_texture.index >= 0
                    ? &m_textures[index_map.texture_ids.at(source_material.pbr_metallic_roughness.metallic_roughness_texture.index)]
                    : nullptr;
                material.setMetallicRoughness(mr);
            }
            if (source_material.normal_texture.index >= 0)
            {
                material.setNormalTexture(&m_textures[index_map.texture_ids.at(source_material.normal_texture.index)]);
            }
            if (source_material.occlusion_texture.index >= 0)
            {
                material.setOcclusionTexture(&m_textures[index_map.texture_ids.at(source_material.occlusion_texture.index)]);
            }
            if (source_material.emissive_texture.index >= 0)
            {
                material.setEmissiveTexture(&m_textures[index_map.texture_ids.at(source_material.emissive_texture.index)]);
            }
            for (auto& [mesh_id, submeshes] : material_attachment.target_meshes)
            {
                Mesh& mesh = m_scene_meshes[mesh_id];
                for (size_t submesh_id : submeshes)
                {
                    Submesh& submesh = mesh.getSubmesh(submesh_id);
                    submesh.setBaseMaterial(new_material_id);
                }
            }
        }
    }

    for (auto const& [node_index_in_scene, data_desc] : index_map.node_attachments)
    {
        Node& n = m_scene_nodes[node_index_in_scene];
        for (NodeDataDesc const& d : data_desc)
        {
            switch (d.attached_data_type)
            {
            case NodeDataType::light:
            {
                uint32_t const light_id = static_cast<uint32_t>(index_map.light_ids.at(d.gltf_attachment_index));
                Light& l = m_lights[light_id];
                n.setLight(light_id);
                m_light_names_lut.emplace(std::make_pair(core::misc::HashedString{ l.getStringName() }, static_cast<size_t>(node_index_in_scene)));
                break;
            }
            case NodeDataType::camera:
            {
                uint32_t const camera_id = static_cast<uint32_t>(index_map.camera_ids.at(d.gltf_attachment_index));
                Camera& c = m_cameras[camera_id];
                n.setCamera(camera_id);
                m_camera_names_lut.emplace(std::make_pair(core::misc::HashedString{ c.getStringName() }, static_cast<size_t>(node_index_in_scene)));
                break;
            }
            case NodeDataType::mesh:
            {
                uint32_t const mesh_id = static_cast<uint32_t>(index_map.mesh_ids.at(d.gltf_attachment_index));
                n.setMesh(mesh_id);
                break;
            }
            case NodeDataType::skin:
            {
                break;
            }
            }
        }
    }

    {
        // Schedule root signature and PSO creation
        auto* p_rs_blob_cache = m_globals.get<core::dx::d3d12::caches::RootSignatureBlobCache>();
        p_rs_blob_cache->createRootSignatures();

        auto* p_pso_blob_cache = m_globals.get<core::dx::d3d12::caches::PSOBlobCache>();
        p_pso_blob_cache->createPipelineStates();
    }

    if(!m_materials.empty())
    {
        // Populate scene parameter data mappings
        m_scene_parameters_data_mapper = std::make_unique<core::dx::d3d12::ConstantBufferDataMapper>(
            m_materials[0].getStaticState().getSceneParametersUniformBufferReflection()
        );
        setCurrentCamera(0);
    }

    buildDraws();

    return load_result;
}

int Scene::readSceneNode(
    tg3_model const& model,
    tg3_node const& node,
    GltfToSceneIndexMap& index_map
)
{
    int new_node_id = static_cast<int>(m_scene_nodes.size());
    m_scene_nodes.emplace_back(this, static_cast<uint32_t>(new_node_id));
    m_scene_nodes[new_node_id].setStringName(std::string(node.name.data, node.name.len));

    if (node.light >= 0)
    {
        index_map.light_ids.insert({ node.light, -1 });
        index_map.node_attachments[new_node_id].push_back(
            {
                .attached_data_type = NodeDataType::light, 
                .gltf_attachment_index = node.light 
            }
        );
    }

    if (node.mesh >= 0)
    {
        // Node contains a mesh, count it towards scene memory size
        index_map.mesh_ids.insert({ node.mesh, -1 });
        index_map.node_attachments[new_node_id].push_back(
            { 
                .attached_data_type = NodeDataType::mesh, 
                .gltf_attachment_index = node.mesh
            }
        );
        tg3_mesh const& mesh = model.meshes[node.mesh];
        for (uint32_t pi = 0; pi < mesh.primitives_count; ++pi)
        {
            tg3_primitive const& p = mesh.primitives[pi];

            if (p.indices >= 0)
            {
                // primitive has index buffer
                tg3_accessor const& accessor = model.accessors[p.indices];
                if (accessor.buffer_view >= 0)
                {
                    tg3_buffer_view const& buffer_view = model.buffer_views[accessor.buffer_view];
                    index_map.buffer_ids.insert({ buffer_view.buffer, -1 });
                }
            }

            for (uint32_t ai = 0; ai < p.attributes_count; ++ai)
            {
                tg3_accessor const& accessor = model.accessors[p.attributes[ai].value];
                if (accessor.buffer_view >= 0)
                {
                    tg3_buffer_view const& buffer_view = model.buffer_views[accessor.buffer_view];
                    index_map.buffer_ids.insert({ buffer_view.buffer, -1 });
                }
            }

            if (p.material >= 0)
            {
                index_map.material_ids.insert({ p.material, -1 });

                tg3_material const& material = model.materials[p.material];
                tg3_pbr_metallic_roughness const& pbr = material.pbr_metallic_roughness;

                if (pbr.base_color_texture.index >= 0)
                {
                    index_map.texture_ids.insert({ pbr.base_color_texture.index, -1 });
                    int sampler_id = model.textures[pbr.base_color_texture.index].sampler;
                    if (sampler_id >= 0) index_map.sampler_ids.insert({ sampler_id, -1 });
                }

                if (pbr.metallic_roughness_texture.index >= 0)
                {
                    index_map.texture_ids.insert({ pbr.metallic_roughness_texture.index, -1 });
                    int sampler_id = model.textures[pbr.metallic_roughness_texture.index].sampler;
                    if (sampler_id >= 0) index_map.sampler_ids.insert({ sampler_id, -1 });
                }

                if (material.normal_texture.index >= 0)
                {
                    index_map.texture_ids.insert({ material.normal_texture.index, -1 });
                    int sampler_id = model.textures[material.normal_texture.index].sampler;
                    if (sampler_id >= 0) index_map.sampler_ids.insert({ sampler_id, -1 });
                }

                if (material.occlusion_texture.index >= 0)
                {
                    index_map.texture_ids.insert({ material.occlusion_texture.index, -1 });
                    int sampler_id = model.textures[material.occlusion_texture.index].sampler;
                    if (sampler_id >= 0) index_map.sampler_ids.insert({ sampler_id, -1 });
                }

                if (material.emissive_texture.index >= 0)
                {
                    index_map.texture_ids.insert({ material.emissive_texture.index, -1 });
                    int sampler_id = model.textures[material.emissive_texture.index].sampler;
                    if (sampler_id >= 0) index_map.sampler_ids.insert({ sampler_id, -1 });
                }
            }

            if (p.indices >= 0 && p.attributes_count > 0 && p.material >= 0)
            {
                ++m_total_submesh_count;
            }
        }
    }

    if (node.camera >= 0)
    {
        index_map.camera_ids.insert({ node.camera, -1 });
        index_map.node_attachments[new_node_id].push_back(
            { 
                .attached_data_type = NodeDataType::camera, 
                .gltf_attachment_index = node.camera
            }
        );
    }

    if (node.skin >= 0)
    {
        index_map.animation_ids.insert({ node.skin, -1 });
        index_map.node_attachments[new_node_id].push_back(
            { 
                .attached_data_type = NodeDataType::skin, 
                .gltf_attachment_index = node.skin
            }
        );
    }

    for (uint32_t i = 0; i < node.children_count; ++i)
    {
        int32_t child_node_index = node.children[i];
        tg3_node const& gltf_node = model.nodes[child_node_index];
        int node_scene_id = readSceneNode(model, gltf_node, index_map);
        m_scene_nodes[new_node_id].addChild(static_cast<uint32_t>(node_scene_id));
    }

    return new_node_id;
}

bool Scene::loadLights(
    tg3_model const& model,
    GltfToSceneIndexMap& index_map
)
{
    auto& light_ids = index_map.light_ids;
    if (m_enabled_extensions[c_khr_light_punctual_ext])
    {
        if (light_ids.empty()) return true;

        m_lights.clear();
        m_lights.reserve(light_ids.size());
        for (auto& [light_id, light_id_in_scene] : light_ids)
        {
            tg3_light const& light = model.lights[light_id];

            // Retrieve light type
            LightType lightType{};
            if (tg3_str_equals_cstr(light.type, "directional"))
            {
                lightType = LightType::directional;
            }
            else if (tg3_str_equals_cstr(light.type, "point"))
            {
                lightType = LightType::point;
            }
            else if (tg3_str_equals_cstr(light.type, "spot"))
            {
                lightType = LightType::spot;
            }
            else
            {
                LEXGINE_LOG_ERROR(this, std::string{ c_khr_light_punctual_ext } + ": light " + std::to_string(light_id) + " has invalid type");
                return false;
            }

            Light sceneLight{ lightType };
            if (light.name.data && light.name.len)
                sceneLight.setStringName(std::string(light.name.data, light.name.len));

            // Color and intensity are always present (defaults: {1,1,1} and 1.0)
            sceneLight.setColor(glm::vec3{
                static_cast<float>(light.color[0]),
                static_cast<float>(light.color[1]),
                static_cast<float>(light.color[2])
                });
            sceneLight.setIntensity(static_cast<float>(light.intensity));

            if (lightType != LightType::point)
            {
                sceneLight.setDirection({ 0.f, 0.f, -1.f });
            }

            switch (lightType)
            {
            case LightType::directional:
                break;
            case LightType::spot:
                // v3 fills outer_cone_angle default (PI/4) at parse time
                sceneLight.setInnerConeAngle(static_cast<float>(light.spot.inner_cone_angle));
                sceneLight.setOuterConeAngle(static_cast<float>(light.spot.outer_cone_angle));
                [[fallthrough]];    // spot lights also support range per KHR_lights_punctual

            case LightType::point:
                // range == 0 means infinite per glTF spec
                if (light.range > 0.0)
                    sceneLight.setRange(static_cast<float>(light.range));
                break;
            }

            light_id_in_scene = m_lights.size();
            m_lights.push_back(sceneLight);
        }
    }

    return true;
}

bool Scene::loadTextures(
    tg3_model const& model,
    GltfToSceneIndexMap& index_map
)
{
    auto& texture_ids = index_map.texture_ids;
    auto& sampler_ids = index_map.sampler_ids;

    conversion::ImageLoaderPool const& image_loader_pool = *m_globals.get<conversion::ImageLoaderPool>();
    conversion::TextureConverter& texture_converter = *m_globals.get<conversion::TextureConverter>();

    m_textures.reserve(texture_ids.size());
    m_samplers.reserve(sampler_ids.size() + 1);   // +1 for default sampler appended below

    for (auto& [sampler_id, sampler_id_in_scene] : sampler_ids)
    {
        tg3_sampler const& sampler = model.samplers[sampler_id];
        sampler_id_in_scene = m_samplers.size();
        m_samplers.emplace_back(
            Sampler{ gltfCast<MinificationFilter>(sampler.min_filter), gltfCast<MagnificationFilter>(sampler.mag_filter),
                gltfCast<WrapMode>(sampler.wrap_s), gltfCast<WrapMode>(sampler.wrap_t) }
        );
    }
    m_samplers.emplace_back(Sampler{});    // default sampler to be used in case GLTF texture does not define one. This sampler is always stored the last in the scene cache

    for (auto& [texture_id, texture_id_in_scene] : texture_ids)
    {
        tg3_texture const& texture = model.textures[texture_id];
        if (texture.source < 0)
        {
            LEXGINE_LOG_ERROR(this, "texture " + std::to_string(texture_id) + " has no image source");
            return false;
        }
        tg3_image const& gltf_image = model.images[texture.source];
        int sampler_id_in_scene = texture.sampler >= 0 ? sampler_ids[texture.sampler] : static_cast<int>(m_samplers.size() - 1);
        texture_id_in_scene = m_textures.size();
        if (gltf_image.image.count == 0)
        {
            // image is loaded from uri
            std::string uri(gltf_image.uri.data, gltf_image.uri.len);
            m_textures.emplace_back(Texture{ .image = Image{m_scene_path.remove_filename() / uri, image_loader_pool}, .sampler_id = sampler_id_in_scene });
        }
        else
        {
            // image is embedded in gltf — copy arena-owned span into a vector
            std::vector<uint8_t> image_data(gltf_image.image.data, gltf_image.image.data + gltf_image.image.count);
            std::string image_name(gltf_image.name.data, gltf_image.name.len);
            m_textures.emplace_back(Texture{
                    .image = Image{
                        std::move(image_data),
                        static_cast<uint32_t>(gltf_image.width),
                        static_cast<uint32_t>(gltf_image.height),
                        static_cast<size_t>(gltf_image.component),
                        static_cast<size_t>(gltf_image.bits),
                        conversion::ImageColorSpace::srgb,
                        image_name,
                        m_timestamp,
                        image_loader_pool
                    },
                    .sampler_id = sampler_id_in_scene
                }
            );
        }
        m_textures.back().p_texture_conversion_task = texture_converter.addTextureConversionTask(m_textures.back().image, false);
    }
    texture_converter.convertTextures();
    texture_converter.uploadTextures();
    texture_converter.waitForTextureUploadCompletion();
    return true;
}

bool Scene::loadMeshes(
    tg3_model const& model,
    GltfToSceneIndexMap& index_map
)
{
    auto& mesh_ids = index_map.mesh_ids;
    auto const& buffer_ids = index_map.buffer_ids;

    core::dx::d3d12::DxgiFormatFetcher const& dxgi_format_fetcher = m_globals.get<core::dx::d3d12::DxResourceFactory>()->dxgiFormatFetcher();

    // Parse meshes
    for (auto& [mesh_id, mesh_id_in_scene] : mesh_ids)
    {
        tg3_mesh const& mesh = model.meshes[mesh_id];

        // Parse mesh primitives
        std::vector<double> morph_weights(mesh.weights, mesh.weights + mesh.weights_count);

        mesh_id_in_scene = m_scene_meshes.size();
        m_scene_meshes.emplace_back(Mesh{ *this, std::string(mesh.name.data, mesh.name.len) });
        Mesh& new_mesh = m_scene_meshes.back();
        new_mesh.applyMorphWeights(morph_weights);

        for (uint32_t pi = 0; pi < mesh.primitives_count; ++pi)
        {
            tg3_primitive const& mesh_primitive = mesh.primitives[pi];
            uint32_t submesh_id = new_mesh.addSubmesh();
            Submesh& submesh = new_mesh.getSubmesh(submesh_id);

            if (mesh_primitive.indices >= 0)
            {
                SceneMemoryBufferHandle index_buffer{};
                core::dx::d3d12::IndexDataType index_type{};

                tg3_accessor const& indices_accessor = model.accessors[mesh_primitive.indices];
                assert(indices_accessor.type == TG3_TYPE_SCALAR);

                tg3_buffer_view const& indices_buffer_view = model.buffer_views[indices_accessor.buffer_view];
                assert(indices_buffer_view.target == TG3_TARGET_ELEMENT_ARRAY_BUFFER);

                switch (indices_accessor.component_type)
                {
                case TG3_COMPONENT_TYPE_INT:
                case TG3_COMPONENT_TYPE_UNSIGNED_INT:
                    index_type = core::dx::d3d12::IndexDataType::_32_bit;
                    break;

                case TG3_COMPONENT_TYPE_SHORT:
                case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index_type = core::dx::d3d12::IndexDataType::_16_bit;
                    break;

                default:
                    assert(false);
                }

                index_buffer = m_scene_memory.getBuffer(buffer_ids.at(indices_buffer_view.buffer));
                index_buffer.offset += indices_buffer_view.byte_offset;
                index_buffer.size = indices_buffer_view.byte_length;

                submesh.setIndexBuffer(index_buffer, index_type);
            }

            int current_buffer = -1;
            int current_vb_slot = -1;
            size_t const invalid_value = std::numeric_limits<size_t>::max();
            size_t current_element_count = invalid_value;
            size_t current_buffer_stride = invalid_value;
            std::array<SceneMemoryBufferHandle, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> vertex_buffers{};

            lexgine::core::VertexAttributeSpecificationList vertex_attributes_for_vb_slot{};
            lexgine::core::VertexAttributeSpecificationList all_vertex_attributes{};
            vertex_attributes_for_vb_slot.reserve(D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

            for (uint32_t ai = 0; ai < mesh_primitive.attributes_count; ++ai)
            {
                std::string attribute_name(mesh_primitive.attributes[ai].key.data, mesh_primitive.attributes[ai].key.len);
                int accessor_id = mesh_primitive.attributes[ai].value;
                tg3_accessor const& accessor = model.accessors[accessor_id];
                assert((accessor.type >= TG3_TYPE_VEC2 && accessor.type <= TG3_TYPE_VEC4)
                    || accessor.type == TG3_TYPE_SCALAR);

                tg3_buffer_view const& buffer_view = model.buffer_views[accessor.buffer_view];
                assert(buffer_view.target == TG3_TARGET_ARRAY_BUFFER);

                if (current_buffer != buffer_view.buffer 
                    || vertex_buffers[current_vb_slot].offset != buffer_view.byte_offset
                    || current_buffer_stride == 0
                    )
                {
                    if (current_buffer >= 0
                        && current_vb_slot >= 0
                        && current_element_count != invalid_value
                        && current_buffer_stride != invalid_value)
                    {
                        submesh.setVertexBuffer(
                            static_cast<size_t>(current_vb_slot),
                            vertex_buffers[current_vb_slot],
                            vertex_attributes_for_vb_slot,
                            current_element_count,
                            current_buffer_stride
                        );
                        vertex_attributes_for_vb_slot.clear();
                    }
                    current_buffer = buffer_view.buffer;
                    ++current_vb_slot;
                    assert(current_vb_slot < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
                    current_element_count = accessor.count;
                    current_buffer_stride = buffer_view.byte_stride;
                    vertex_buffers[current_vb_slot] = m_scene_memory.getBuffer(buffer_ids.at(current_buffer));
                    vertex_buffers[current_vb_slot].offset = buffer_view.byte_offset;
                    vertex_buffers[current_vb_slot].size = buffer_view.byte_length;
                }

                auto [va_name, va_index] = extractNameAndIndexFromAttributeName(attribute_name);
                core::dx::d3d12::DxgiFormatFetcher::va_spec vertex_attribute_desc
                {
                    .format = gltfCast<lexgine::core::misc::DataFormat>(accessor.component_type),
                    .element_count = static_cast<unsigned char>(tg3_num_components(accessor.type)),
                    .is_normalized = accessor.normalized != 0,
                    .primitive_assembler_input_slot = static_cast<unsigned char>(current_vb_slot),
                    .element_offset = static_cast<uint32_t>(accessor.byte_offset),
                    .name = va_name.c_str(),
                    .name_index = static_cast<uint32_t>(va_index),
                    .instancing_data_rate = 0
                };
                auto vertex_attribute = dxgi_format_fetcher.createVertexAttribute(vertex_attribute_desc);
                vertex_attributes_for_vb_slot.push_back(vertex_attribute);
                all_vertex_attributes.push_back(vertex_attribute);
            }
            {
                submesh.setVertexBuffer(
                    static_cast<size_t>(current_vb_slot),
                    vertex_buffers[current_vb_slot],
                    vertex_attributes_for_vb_slot,
                    current_element_count,
                    current_buffer_stride
                );
            }
            
            
            if (mesh_primitive.material >= 0)
            {
                int32_t material_id = mesh_primitive.material;
                tg3_material const& gltf_source_material = model.materials[material_id];
                auto material_static_state_create_info_it = 
                    registerMaterialStaticState(gltf_source_material, all_vertex_attributes);
                auto material_attachment_it = m_material_attachements.find(material_id);
                MaterialAttachment& attachment = m_material_attachements[material_id];
                attachment.create_info_it = material_static_state_create_info_it;
                attachment.target_meshes[mesh_id_in_scene].push_back(submesh_id);
            }
        }
    }

    return true;
}

bool Scene::loadCameras(tg3_model const& model, GltfToSceneIndexMap& index_map)
{
    for (auto& [camera_id, camera_id_in_scene] : index_map.camera_ids)
    {
        tg3_camera const& camera = model.cameras[camera_id];
        Camera sceneCamera{ std::string(camera.name.data, camera.name.len) };
        ProjectionType cameraProjectionType{};
        if (tg3_str_equals_cstr(camera.type, "perspective"))
        {
            cameraProjectionType = ProjectionType::Perspective;
            tg3_perspective_camera const& perspectiveCamera = camera.perspective;
            sceneCamera.setPerspective(
                static_cast<float>(perspectiveCamera.yfov),
                static_cast<float>(perspectiveCamera.aspect_ratio),
                static_cast<float>(perspectiveCamera.znear),
                static_cast<float>(perspectiveCamera.zfar),
                m_global_settings.isInverseDepthClipSpaceEnabled()
            );
        }
        else if (tg3_str_equals_cstr(camera.type, "orthographic"))
        {
            cameraProjectionType = ProjectionType::Orthographic;
            tg3_orthographic_camera const& orthographicCamera = camera.orthographic;
            sceneCamera.setOrthographic(
                static_cast<float>(-orthographicCamera.xmag * .5),
                static_cast<float>(orthographicCamera.xmag * .5),
                static_cast<float>(orthographicCamera.ymag * .5),
                static_cast<float>(-orthographicCamera.ymag * .5),
                static_cast<float>(orthographicCamera.znear),
                static_cast<float>(orthographicCamera.zfar)
            );
        }
        else
        {
            return false;
        }

        camera_id_in_scene = static_cast<int>(m_cameras.size());
        m_cameras.push_back(sceneCamera);
    }
    return true;
}

bool Scene::loadAnimations(tg3_model const& model, GltfToSceneIndexMap& index_map)
{
    return true;
}

Scene::MaterialStaticStateCreateInfoSet::const_iterator
    Scene::registerMaterialStaticState(
        tg3_material const& gltf_material,
        const lexgine::core::VertexAttributeSpecificationList& vertex_attributes
    )
{
    std::list<core::dx::dxcompilation::HLSLMacroDefinition> defines{};
    tg3_str const& alpha_mode = gltf_material.alpha_mode;
    defines.push_back({ .name = std::string{alpha_mode.data, alpha_mode.data + alpha_mode.len} });

    MaterialStaticStateCreateInfo material_ss_create_info{};

    auto* p_hlsl_shader_blob_cache = m_globals.get<core::dx::d3d12::caches::HLSLShaderBlobCache>();
    {
        // Vertex shader
        lexgine::core::dx::d3d12::caches::HLSLFileTranslationUnit translation_unit_vs{ m_globals, "pbr.vs", "pbr.vs.hlsl" };
        material_ss_create_info.vertex_shader = p_hlsl_shader_blob_cache->createHLSLShaderBlobCompilationContract(
            translation_unit_vs,
            lexgine::core::dx::dxcompilation::ShaderModel::model_62,
            lexgine::core::dx::dxcompilation::ShaderType::vertex,
            "VSMain",
            defines
        );
    }
    {
        // Pixel shader
        lexgine::core::dx::d3d12::caches::HLSLFileTranslationUnit translation_unit_ps{ m_globals, "pbr.ps", "pbr.ps.hlsl" };
        material_ss_create_info.pixel_shader = p_hlsl_shader_blob_cache->createHLSLShaderBlobCompilationContract(
            translation_unit_ps,
            lexgine::core::dx::dxcompilation::ShaderModel::model_62,
            lexgine::core::dx::dxcompilation::ShaderType::pixel,
            "PSMain",
            defines
        );
    }
    material_ss_create_info.vertex_data_format = vertex_attributes;
    auto [it, _] = m_material_static_state_create_infos.insert(material_ss_create_info);
    return it;
}

void Scene::buildDraws()
{
    for (Node& n : m_scene_nodes)
    {
        uint32_t mesh_id = n.getMesh();
        if (mesh_id != c_scene_resource_invalid_id)
        {
            Mesh& mesh = m_scene_meshes[mesh_id];
            for (uint32_t submesh_id = 0; submesh_id < mesh.getSubmeshCount(); ++submesh_id)
            {
                Submesh& submesh = mesh.getSubmesh(submesh_id);
                uint32_t material_id = submesh.getBaseMaterial();
                if (material_id != c_scene_resource_invalid_id)
                {
                    DrawKey instance{ .mesh_id = mesh_id, .submesh_id = submesh_id };
                    uint32_t draw_query_id = registerDrawQuery(material_id);
                    uint32_t draw_id = registerDraw(draw_query_id, instance);
                    registerInstance(n, draw_id);
                }
            }
        }
    }

    m_instances_gpu_data.resize(m_instances_cpu_data.size());
}

uint32_t Scene::registerDrawQuery(uint32_t material_id)
{
    {
        auto it = m_material_to_draw_query_lut.find(material_id);
        if (it != m_material_to_draw_query_lut.end())
        {
            return it->second;
        }
    }
    DrawQuery new_draw_query{ .material_id = material_id };
    uint32_t draw_query_id = static_cast<uint32_t>(m_draw_queries.size());
    m_draw_queries.push_back(new_draw_query);
    m_material_to_draw_query_lut[material_id] = draw_query_id;
    return draw_query_id;
}

uint32_t Scene::registerDraw(uint32_t draw_query_id, DrawKey const& draw_key)
{
    {
        auto it = m_draw_key_to_draw_lut.find(draw_key);
        if (it != m_draw_key_to_draw_lut.end())
        {
            uint32_t draw_id = it->second;
            assert(m_draws[draw_id].draw_query_id == draw_query_id);
            return draw_id;
        }
    }

    DrawQuery& draw_query = m_draw_queries[draw_query_id];

    uint32_t draw_id = static_cast<uint32_t>(m_draws.size());
    draw_query.draw_ids.push_back(draw_id);

    Draw new_draw { 
        .draw_query_id = draw_query_id, 
        .draw_instance_id = draw_key, 
        .gpu_instancing_section_start_index = c_scene_resource_invalid_id 
    };
    m_draws.push_back(new_draw);
    m_draw_key_to_draw_lut[draw_key] = draw_id;

    return draw_id;
}

uint32_t Scene::registerInstance(Node& instance_owning_node, uint32_t parent_draw_id)
{
    Draw& draw = m_draws[parent_draw_id];

    uint32_t instance_id = static_cast<uint32_t>(m_instances_cpu_data.size());
    draw.instance_indices.push_back(instance_id);

    PerInstanceCpuData new_instance{ .owning_node_id = instance_owning_node.getSelfId(), .draw_id = parent_draw_id };
    m_instances_cpu_data.push_back(new_instance);

    return instance_id;
}

#pragma endregion


}


