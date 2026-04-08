
#include <algorithm>
#include <cctype>
#include <future>
#include <thread>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#include <tinygltf/tiny_gltf.h>
#undef TINYGLTF_IMPLEMENTATION
#undef TINYGLTF_NO_STB_IMAGE_WRITE
#undef TINYGLTF_NO_STB_IMAGE

#include <glm/gtc/constants.hpp>
#include <engine/core/globals.h>
#include <engine/core/misc/misc.h>
#include <engine/core/dx/d3d12/dx_resource_factory.h>
#include <engine/core/dx/d3d12/basic_rendering_services.h>
#include <engine/core/concurrency/task_graph.h>
#include <engine/core/concurrency/task_sink.h>
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

int getSceneIndexFromName(tinygltf::Model const& model, std::string const& scene_name)
{
    for (size_t i = 0; i < model.scenes.size(); ++i) {
        if (model.scenes[i].name == scene_name) {
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
    case TINYGLTF_COMPONENT_TYPE_BYTE:
        return lexgine::core::misc::DataFormat::int8;

    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return lexgine::core::misc::DataFormat::uint8;

    case TINYGLTF_COMPONENT_TYPE_SHORT:
        return lexgine::core::misc::DataFormat::int16;

    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return lexgine::core::misc::DataFormat::uint16;

    case TINYGLTF_COMPONENT_TYPE_INT:
        return lexgine::core::misc::DataFormat::int32;

    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return lexgine::core::misc::DataFormat::uint32;

    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return lexgine::core::misc::DataFormat::float32;

    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
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
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return MinificationFilter::nearest;

    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        return MinificationFilter::linear;

    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        return MinificationFilter::nearest_mipmap_nearest;

    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        return MinificationFilter::linear_mipmap_nearest;

    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        return MinificationFilter::nearest_mipmap_linear;

    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        return MinificationFilter::linear_mipmap_linear;

    default:
        return MinificationFilter::linear_mipmap_linear;
    }
}

template <>
lexgine::scenegraph::MagnificationFilter gltfCast<lexgine::scenegraph::MagnificationFilter>(int gltf_magnification_filter)
{
    switch (gltf_magnification_filter) {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return MagnificationFilter::nearest;

    case TINYGLTF_TEXTURE_FILTER_LINEAR:
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
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return WrapMode::repeat;

    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return WrapMode::mirrored_repeat;

    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return WrapMode::clamp_to_edge;

    default:
        return WrapMode::repeat;
    }
}

std::pair<std::string, unsigned> extractNameAndIndexFromAttributeName(std::string const& attribute_name)
{
    int name_length = attribute_name.find_last_not_of("0123456789") + 1;
    std::string name = attribute_name.substr(0, name_length);
    std::vector<char> uppercase_name; uppercase_name.resize(name.length());
    std::transform(name.begin(), name.end(), uppercase_name.begin(), [](char e) { return static_cast<char>(std::toupper(e)); });
    name = std::string{ uppercase_name.data(), uppercase_name.size() };
    unsigned index = (name_length < attribute_name.size()) ? static_cast<unsigned>(std::stoul(attribute_name.substr(name_length))) : 0;
    return { name, index };
}

}


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
    std::unique_ptr<tinygltf::Model> gltf_model = readGltfModel(path_to_scene);
    if (!gltf_model)
    {
        return;
    }
    setStringName(gltf_model->scenes[scene_id].name);
    m_scene_source_parse_status = readScene(*gltf_model, scene_id);
}

Scene::Scene(
    core::Globals& globals, 
    core::dx::d3d12::BasicRenderingServices& basic_rendering_services,
    std::filesystem::path const& path_to_scene, 
    std::string const& scene_name
)
    : m_globals{ globals }
    , m_basic_rendering_services{ basic_rendering_services }
    , m_timestamp{ fetchTimestamp(path_to_scene) }
    , m_scene_path{ path_to_scene }
{
    std::unique_ptr<tinygltf::Model> gltf_model = readGltfModel(path_to_scene);
    if (!gltf_model)
    {
        return;
    }

    m_scene_index = getSceneIndexFromName(*gltf_model, scene_name);
    setStringName(scene_name);

    if (m_scene_index == -1)
    {
        LEXGINE_LOG_ERROR(this, "Unable to load gltf file '" + path_to_scene.string() + "': scene with name '" + scene_name + "' not found");
        m_scene_source_parse_status = false;
        return;
    }

    m_scene_source_parse_status = readScene(*gltf_model, m_scene_index);
}

bool Scene::loadStatus() const
{
    if (!m_scene_source_parse_status) return false;
    conversion::TextureConverter& texture_converter = *m_globals.get<conversion::TextureConverter>();
    return !m_material_construction_task_sink->isRunning()
        && texture_converter.isTextureConversionCompleted()
        && texture_converter.isTextureUploadCompleted();
}

[[nodiscard]]
std::unique_ptr<tinygltf::Model> Scene::readGltfModel(std::filesystem::path const& path)
{
    tinygltf::TinyGLTF gltf_loader {};
    std::unique_ptr<tinygltf::Model> gltf_model = std::make_unique<tinygltf::Model>();

    std::string gltf_path_to_file = path.string();
	{
		std::string extension = path.extension().string();
        std::vector<char> buf(extension.size());
        std::transform(extension.begin(), extension.end(), buf.begin(), [](char c) {return static_cast<char>(std::tolower(c)); });
        extension = std::string{ buf.begin(), buf.end() };
        if (extension == ".ktx")
        {
            m_scene_source = SceneSource::gltf;
        }
        else if (extension == ".glb")
        {
            m_scene_source = SceneSource::glb;
        }
        else
        {
            LEXGINE_LOG_ERROR(this, std::format("Unable to load scene '{}': scene file has unsupported format extension '{}'", gltf_path_to_file, extension));
            return nullptr;
        }
	}

    std::string error_buffer {}, warning_buffer {};
    bool result{ false };
    switch (m_scene_source)
    {
    case SceneSource::gltf:
        result = gltf_loader.LoadASCIIFromFile(gltf_model.get(), &error_buffer, &warning_buffer, gltf_path_to_file);
        break;
    case SceneSource::glb:
        result = gltf_loader.LoadBinaryFromFile(gltf_model.get(), &error_buffer, &warning_buffer, gltf_path_to_file);
        break;
    default:
        LEXGINE_ASSUME;
    }
    
    if (!result || !error_buffer.empty()) {
        LEXGINE_LOG_ERROR(this, error_buffer.c_str());
        return gltf_model;
    }

    if (!warning_buffer.empty()) {
        logger().out(warning_buffer, core::misc::LogMessageType::exclamation);
    }

    return gltf_model;
}

bool Scene::readScene(tinygltf::Model& model, unsigned scene_index)
{
    std::string gltf_path_to_file = m_scene_path.string();

    // Check used extensions
    for (auto& ext : model.extensionsUsed)
    {
        auto it = m_enabled_extensions.find(ext);
        if (it == m_enabled_extensions.end())
        {
            if (std::find(model.extensionsRequired.begin(), model.extensionsRequired.end(), ext) != model.extensionsRequired.end())
            {
                LEXGINE_LOG_ERROR(this, "Unable to load gltf file '" + gltf_path_to_file + "': required extension " + ext.c_str() + " is not supported");
                return false;
            }
            core::misc::Log::retrieve()->out("gltf file '" + gltf_path_to_file + "' contains unsupported extension " + ext.c_str(), core::misc::LogMessageType::exclamation);
        }
        else
        {
            it->second = true;
        }
    }


    std::unordered_map<int, int> scene_light_ids;
    std::unordered_map<int, int> scene_material_ids;
    std::unordered_map<int, int> scene_mesh_ids;
    std::unordered_map<int, int> scene_camera_ids;
    std::unordered_map<int, int> scene_animation_ids;
    std::unordered_map<int, int> scene_buffer_ids;
    std::unordered_map<int, int> scene_texture_ids;
    std::unordered_map<int, int> scene_sampler_ids;
    {
        tinygltf::Scene& scene = model.scenes[scene_index];
        
        for (int node_id : scene.nodes)
        {   
            tinygltf::Node& node = model.nodes[node_id];
            m_scene_nodes.emplace_back();
            m_scene_nodes.back().setStringName(node.name);

            if (node.light >= 0)
            {
                scene_light_ids.insert({ node.light, -1 });
            }

            if (node.mesh >= 0)
            {
                // Node contains a mesh, count it towards scene memory size
                scene_mesh_ids.insert({ node.mesh, -1 });
                tinygltf::Mesh& mesh = model.meshes[node.mesh];
                for (auto& p : mesh.primitives)
                {
                    if (p.indices >= 0)
                    {
                        // primitive has index buffer
                        tinygltf::Accessor const& accessor = model.accessors[p.indices];
                        if (accessor.bufferView >= 0)
                        {
                            tinygltf::BufferView const& buffer_view = model.bufferViews[accessor.bufferView];
                            int buffer_id = buffer_view.buffer;
                            scene_buffer_ids.insert({ buffer_id, -1 });
                        }
                    }

                    for (auto const& attr : p.attributes)
                    {
                        tinygltf::Accessor const& accessor = model.accessors[attr.second];
                        if (accessor.bufferView >= 0) 
                        {
                            tinygltf::BufferView const& buffer_view = model.bufferViews[accessor.bufferView];
                            int buffer_id = buffer_view.buffer;
                            scene_buffer_ids.insert({ buffer_id, -1 });
                        }
                    }

					if (p.material >= 0)
					{
						scene_material_ids.insert({ p.material, -1 });

						tinygltf::Material& material = model.materials[p.material];

						tinygltf::PbrMetallicRoughness& pbr_metallic_roughness = material.pbrMetallicRoughness;
						if (pbr_metallic_roughness.baseColorTexture.index >= 0)
						{
							scene_texture_ids.insert({ pbr_metallic_roughness.baseColorTexture.index, -1 });
							int sampler_id = model.textures[pbr_metallic_roughness.baseColorTexture.index].sampler;
							if (sampler_id >= 0)
							{
								scene_sampler_ids.insert({ sampler_id, -1 });
							}
						}

						if (pbr_metallic_roughness.metallicRoughnessTexture.index >= 0)
						{
							scene_texture_ids.insert({ pbr_metallic_roughness.metallicRoughnessTexture.index, -1 });
							int sampler_id = model.textures[pbr_metallic_roughness.metallicRoughnessTexture.index].sampler;
							if (sampler_id >= 0)
							{
								scene_sampler_ids.insert({ sampler_id, -1 });
							}
						}

						if (material.normalTexture.index >= 0)
						{
							scene_texture_ids.insert({ material.normalTexture.index, -1 });
							int sampler_id = model.textures[material.normalTexture.index].sampler;
							if (sampler_id >= 0)
							{
								scene_sampler_ids.insert({ sampler_id, -1 });
							}
						}

						if (material.occlusionTexture.index >= 0)
						{
							scene_texture_ids.insert({ material.occlusionTexture.index, -1 });
							int sampler_id = model.textures[material.occlusionTexture.index].sampler;
							if (sampler_id >= 0)
							{
								scene_sampler_ids.insert({ sampler_id, -1 });
							}
						}

						if (material.emissiveTexture.index >= 0)
						{
							scene_texture_ids.insert({ material.emissiveTexture.index, -1 });
							int sampler_id = model.textures[material.emissiveTexture.index].sampler;
							if (sampler_id >= 0)
							{
								scene_sampler_ids.insert({ sampler_id, -1 });
							}
						}
					}
                }
            }

            if (node.camera >= 0)
            {
                scene_camera_ids.insert({ node.camera, -1 });
            }

            if (node.skin >= 0)
            {
                scene_animation_ids.insert({ node.skin, -1 });
            }
        }
    }


    // Prepare scene memory
    {
        uint64_t scene_memory_size = std::accumulate(scene_buffer_ids.cbegin(), scene_buffer_ids.cend(), 0ui64,
            [&model](uint64_t acc, std::pair<int, int> const& e) {
                return acc + static_cast<uint64_t>(model.buffers[e.first].data.size());
            });

        m_scene_memory.scene_memory_buffer.reset(new SceneMeshMemory{ m_globals, scene_memory_size });

        for (auto& [buffer_id, buffer_id_in_scene] : scene_buffer_ids)
        {
            buffer_id_in_scene = m_scene_memory.m_scene_memory_handles.size();
            tinygltf::Buffer const& buffer = model.buffers[buffer_id];
            m_scene_memory.m_scene_memory_handles.push_back(
                m_scene_memory.scene_memory_buffer->addData(
                    buffer.data.data(),
                    buffer.data.size()
                )
            );
        }
        m_scene_memory.scene_memory_buffer->uploadAllData();    // Upload all remaining scheduled data as soon as possible (some data may have already been uploaded depending on the size of the scene and the size of staging buffer)
    }

    bool load_result = true;
    if (!loadLights(
        model, 
        scene_light_ids
    ))
    {
        LEXGINE_LOG_ERROR(this, "Unable to load lights when reading scene source \"" + m_scene_path.string() + "\"");
        load_result = false;
    }
    if (!loadTextures(
        model, 
        scene_texture_ids, 
        scene_sampler_ids
    ))
    {
        LEXGINE_LOG_ERROR(this, "Unable to load textures when reading scene source \"" + m_scene_path.string() + "\"");
        load_result = false;
    }
    if (!loadMeshes(
        model,
        scene_mesh_ids,
        scene_buffer_ids
    ))
    {
        LEXGINE_LOG_ERROR(this, "Unable to load meshes when reading scene source \"" + m_scene_path.string() + "\"");
        load_result = false;
    }

    scheduleMaterialConstruction();

    return load_result;
}


bool Scene::loadLights(
    tinygltf::Model const& model, 
    std::unordered_map<int, int>& light_ids
)
{
    if (m_enabled_extensions[c_khr_light_punctual_ext])
    {
        if (light_ids.empty()) return true;

        if (model.extensions.find(c_khr_light_punctual_ext) == model.extensions.end()
            || !model.extensions.at(c_khr_light_punctual_ext).Has("lights"))
        {
            return false;
        }

        auto& khrLights = model.extensions.at(c_khr_light_punctual_ext).Get("lights");
        m_lights.clear();
        m_lights.reserve(light_ids.size());
        for (auto& [light_id, light_id_in_scene] : light_ids)
        {
            auto& light = khrLights.Get(static_cast<int>(light_id));

            // Retrieve light type
            if (!light.Has("type")) {
                LEXGINE_LOG_ERROR(this, std::string{ c_khr_light_punctual_ext } + ": light " + std::to_string(light_id) + " does not have a type");
                return false;
            }

            LightType lightType{};
            {
                std::string khrLightType = light.Get("type").Get<std::string>();
                if (khrLightType == "directional")
                {
                    lightType = LightType::directional;
                }
                else if (khrLightType == "point")
                {
                    lightType = LightType::point;
                }
                else if (khrLightType == "spot")
                {
                    lightType = LightType::spot;
                }
                else
                {
                    LEXGINE_LOG_ERROR(this, std::string{ c_khr_light_punctual_ext } + ": light " + std::to_string(light_id) + " has invalid type");
                    return false;
                }
            }

            Light lexgineLight{ lightType };
            if (light.Has("name"))
                lexgineLight.setStringName(light.Get("name").Get<std::string>());


            // Retrieve light properties
            if (light.Has("color"))
            {
                auto& light_color_property = light.Get("color");
                glm::vec3 color{
                    static_cast<float>(light_color_property.Get(0).Get<double>()),
                    static_cast<float>(light_color_property.Get(1).Get<double>()),
                    static_cast<float>(light_color_property.Get(2).Get<double>())
                };
                lexgineLight.setColor(color);
            }

            if (light.Has("intensity"))
            {
                lexgineLight.setIntensity(static_cast<float>(light.Get("intensity").Get<double>()));
            }

            if (lightType != LightType::point)
            {
                lexgineLight.setDirection({ 0.f, 0.f, -1.f });
            }

            switch (lightType)
            {
            case LightType::directional:
                break;
            case LightType::spot:
            {
                if (!light.Has("spot"))
                {
                    LEXGINE_LOG_ERROR(this, std::string{ c_khr_light_punctual_ext } + ": invalid description of spot light " + std::to_string(light_id) + ", the light does not define a 'spot' property, which is required");
                    return false;
                }

                auto& light_spot_property = light.Get("spot");
                lexgineLight.setInnerConeAngle(static_cast<float>(light_spot_property.Get("innerConeAngle").Get<double>()));

                if (light_spot_property.Has("outerConeAngle"))
                {
                    lexgineLight.setOuterConeAngle(static_cast<float>(light_spot_property.Get("outerConeAngle").Get<double>()));
                }
                else
                {
                    lexgineLight.setOuterConeAngle(glm::pi<float>() / 4.f);
                }
                [[fallthrough]];    // spot lights also support range per KHR_lights_punctual
            }

            case LightType::point:
                if (light.Has("range"))
                    lexgineLight.setRange(static_cast<float>(light.Get("range").Get<double>()));
                break;

            }

            m_lights.push_back(lexgineLight);
            light_id_in_scene = m_lights.size() - 1;
        }
    }

    return true;
}

bool Scene::loadTextures(
    tinygltf::Model& model,
    std::unordered_map<int, int>& texture_ids,
    std::unordered_map<int, int>& sampler_ids
)
{
    conversion::ImageLoaderPool const& image_loader_pool = *m_globals.get<conversion::ImageLoaderPool>();
    conversion::TextureConverter& texture_converter = *m_globals.get<conversion::TextureConverter>();
    
    m_textures.reserve(texture_ids.size());
    m_samplers.reserve(sampler_ids.size() + 1);   // +1 for default sampler appended below

    for (auto& [sampler_id, sampler_id_in_scene] : sampler_ids)
    {
        tinygltf::Sampler& sampler = model.samplers[sampler_id];
        sampler_id_in_scene = m_samplers.size();
        m_samplers.emplace_back(
            Sampler{ gltfCast<MinificationFilter>(sampler.minFilter), gltfCast<MagnificationFilter>(sampler.magFilter),
                gltfCast<WrapMode>(sampler.wrapS), gltfCast<WrapMode>(sampler.wrapT) }
        );
    }
    m_samplers.emplace_back(Sampler{});    // default sampler to be used in case GLTF texture does not define one. This sampler is always stored the last in the scene cache
    
    for (auto& [texture_id, texture_id_in_scene] : texture_ids)
    {
        tinygltf::Texture& texture = model.textures[texture_id];
        if (texture.source < 0)
        {
            LEXGINE_LOG_ERROR(this, "texture " + std::to_string(texture_id) + " has no image source");
            return false;
        }
        tinygltf::Image& gltf_image = model.images[texture.source];
        int sampler_id_in_scene = texture.sampler >= 0 ? sampler_ids[texture.sampler] : static_cast<int>(m_samplers.size() - 1);
        texture_id_in_scene = m_textures.size();
        if (gltf_image.image.empty())
        {
            // image is loaded from uri
            m_textures.emplace_back(Texture{ .image = Image{m_scene_path / gltf_image.uri, image_loader_pool}, .sampler_id = sampler_id_in_scene });
        }
        else
        {
            // image is embedded in gltf
            m_textures.emplace_back(Texture{
                    .image = Image{
                        std::move(gltf_image.image),
                        static_cast<uint32_t>(gltf_image.width),
                        static_cast<uint32_t>(gltf_image.height),
                        static_cast<size_t>(gltf_image.component),
                        static_cast<size_t>(gltf_image.bits),
                        conversion::ImageColorSpace::srgb,
                        gltf_image.name,
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

    return true;
}

bool Scene::loadMeshes(
    tinygltf::Model const& model, 
    std::unordered_map<int, int>& mesh_ids,
    std::unordered_map<int, int> const& buffer_ids
)
{
    core::dx::d3d12::DxgiFormatFetcher const& dxgi_format_fetcher = m_globals.get<core::dx::d3d12::DxResourceFactory>()->dxgiFormatFetcher();

    // Parse meshes
    for (auto& [mesh_id, mesh_id_in_scene] : mesh_ids)
    {
        tinygltf::Mesh const& mesh = model.meshes[mesh_id];

        // Parse mesh primitives
        auto& morph_weights = mesh.weights;

        m_scene_meshes.emplace_back(Mesh{ mesh.name });
        m_scene_meshes.back().applyMorphWeights(morph_weights);

        for (tinygltf::Primitive const& mesh_primitive : mesh.primitives)
        {
            Submesh submesh{ *m_scene_memory.scene_memory_buffer };
            VertexBufferView* vb_view = submesh.getVertexBufferView();
            
            if(mesh_primitive.indices >= 0)
            {
				SceneMemoryBufferHandle index_buffer{};
				IndexType index_type{};

                tinygltf::Accessor const& indices_accessor = model.accessors.at(mesh_primitive.indices);
                assert(indices_accessor.type == TINYGLTF_TYPE_SCALAR);

                tinygltf::BufferView const& indices_buffer_view = model.bufferViews.at(indices_accessor.bufferView);
                assert(indices_buffer_view.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);

                switch (indices_accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_INT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index_type = IndexType::_default;
                    break;

                case TINYGLTF_COMPONENT_TYPE_SHORT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index_type = IndexType::_short;
                    break;

                default:
                    assert(false);
                }

                index_buffer = m_scene_memory.getBuffer(buffer_ids.at(indices_buffer_view.buffer));
                index_buffer.offset += indices_buffer_view.byteOffset;
                index_buffer.size = indices_buffer_view.byteLength;

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

            for (auto p = mesh_primitive.attributes.begin(), end = mesh_primitive.attributes.end(); p != end; ++p)
            {
                auto [attribute_name, accessor_id] = *p;
                tinygltf::Accessor const& accessor = model.accessors.at(accessor_id);
                assert(accessor.type >= TINYGLTF_TYPE_VEC2 && accessor.type <= TINYGLTF_TYPE_VEC4
                    || accessor.type == TINYGLTF_TYPE_SCALAR);

                tinygltf::BufferView const& buffer_view = model.bufferViews.at(accessor.bufferView);
                assert(buffer_view.target == TINYGLTF_TARGET_ARRAY_BUFFER);

                if (current_buffer != buffer_view.buffer)
                {
                    if (current_buffer >= 0
                        && current_vb_slot >= 0
                        && current_element_count != invalid_value 
                        && current_buffer_stride != invalid_value)
                    {
						vb_view->setVertexBuffer(
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
                    current_buffer_stride = buffer_view.byteStride;
					vertex_buffers[current_vb_slot] = m_scene_memory.getBuffer(buffer_ids.at(current_buffer));
					vertex_buffers[current_vb_slot].offset += buffer_view.byteOffset;
					vertex_buffers[current_vb_slot].size = buffer_view.byteLength;
                }

                auto [va_name, va_index] = extractNameAndIndexFromAttributeName(attribute_name);
                core::dx::d3d12::DxgiFormatFetcher::va_spec vertex_attribute_desc
                {
                    .format = gltfCast<lexgine::core::misc::DataFormat>(accessor.componentType),
                    .element_count = static_cast<unsigned char>(tinygltf::GetNumComponentsInType(accessor.type)),
                    .is_normalized = accessor.normalized,
                    .primitive_assembler_input_slot = static_cast<unsigned char>(current_vb_slot),
                    .element_offset = static_cast<uint32_t>(accessor.byteOffset),
                    .name = va_name.c_str(),
                    .name_index = static_cast<uint32_t>(va_index),
                    .instancing_data_rate = 0
                };
                auto vertex_attribute = dxgi_format_fetcher.createVertexAttribute(vertex_attribute_desc);
                vertex_attributes_for_vb_slot.push_back(vertex_attribute);
                all_vertex_attributes.push_back(vertex_attribute);
            }
            {
                vb_view->setVertexBuffer(
                    static_cast<size_t>(current_vb_slot),
                    m_scene_memory.getBuffer(buffer_ids.at(current_buffer)),
                    vertex_attributes_for_vb_slot,
                    current_element_count,
                    current_buffer_stride
                );
            }

			if (mesh_primitive.material >= 0)
			{
				bool result = loadMaterial(model.materials[mesh_primitive.material], all_vertex_attributes);
				if (result)
				{
                    Material& last_loaded_material = m_materials.back();
                    submesh.setBaseMaterial(&last_loaded_material);
				}
                else
                {
					LEXGINE_LOG_ERROR(this, "Unable to load material (id = "
						+ std::to_string(mesh_primitive.material) + ") for mesh "
						+ mesh.name);
                }
			}

            m_scene_meshes.back().addSubmesh(std::move(submesh));
        }
    }

    return true;
}

bool Scene::loadMaterial(const tinygltf::Material& gltfMaterial,
    const lexgine::core::VertexAttributeSpecificationList& vertex_attributes)
{
    if (gltfMaterial.alphaMode != "OPAQUE")
        return false;

    MaterialPSOCompilationContext context{ vertex_attributes };
    MaterialShaderDesc shader_desc{};
    auto* p_hlsl_compilation_task_cache = m_globals.get<core::dx::d3d12::task_caches::HLSLCompilationTaskCache>();
    
	{
		lexgine::core::dx::d3d12::task_caches::HLSLFileTranslationUnit translation_unit_vs{ m_globals, "pbr.vs", "pbr.vs.hlsl" };
		shader_desc.p_vertex_shader_compilation_task = p_hlsl_compilation_task_cache->findOrCreateTask(
			translation_unit_vs,
			lexgine::core::dx::dxcompilation::ShaderModel::model_62,
			lexgine::core::dx::dxcompilation::ShaderType::vertex,
			"VSMain"
		);
	}

	{
		lexgine::core::dx::d3d12::task_caches::HLSLFileTranslationUnit translation_unit_ps{ m_globals, "pbr.ps", "pbr.ps.hlsl" };
		shader_desc.p_pixel_shader_compilation_task = p_hlsl_compilation_task_cache->findOrCreateTask(
			translation_unit_ps,
			lexgine::core::dx::dxcompilation::ShaderModel::model_62,
			lexgine::core::dx::dxcompilation::ShaderType::pixel,
			"PSMain"
		);
	}
    m_material_construction_tasks.push_back(std::make_unique<MaterialAssemblyTask>(m_basic_rendering_services, context, shader_desc));
    
    m_materials.emplace_back(*m_material_construction_tasks.back());
    Material& new_material = m_materials.back();
    new_material.setStringName(gltfMaterial.name);
    new_material.setEmissiveFactor(lexgine::core::math::Vector3f{ gltfMaterial.emissiveFactor[0], gltfMaterial.emissiveFactor[1], gltfMaterial.emissiveFactor[2] });
    new_material.setAlphaMode(AlphaMode::opaque);
    new_material.setAlphaCutoff(gltfMaterial.alphaCutoff);
    new_material.setDoubleSided(gltfMaterial.doubleSided);

    {
        // Metallic-roughness
        Material::MetallicRoughness mr{};
        mr.base_color_factor = lexgine::core::math::Vector4f{
            gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
            gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
            gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
            gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]
        };
        mr.metallic_factor = gltfMaterial.pbrMetallicRoughness.metallicFactor;
        mr.roughness_factor = gltfMaterial.pbrMetallicRoughness.roughnessFactor;
        mr.p_base_color = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0 ? &m_textures[gltfMaterial.pbrMetallicRoughness.baseColorTexture.index] : nullptr;
        mr.p_metallic_roughness = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 ? &m_textures[gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index] : nullptr;
        new_material.setMetallicRoughness(mr);
    }

    if (gltfMaterial.normalTexture.index >= 0)
    {
        new_material.setNormalTexture(&m_textures[gltfMaterial.normalTexture.index]);
    }

    if (gltfMaterial.occlusionTexture.index >= 0)
    {
        new_material.setOcclusionTexture(&m_textures[gltfMaterial.occlusionTexture.index]);
    }

    if (gltfMaterial.emissiveTexture.index >= 0)
    {
        new_material.setEmissiveTexture(&m_textures[gltfMaterial.emissiveTexture.index]);
    }

    return true;
}

void Scene::scheduleMaterialConstruction()
{
    std::unordered_set<core::concurrency::TaskGraphRootNode const*> root_nodes{};
    root_nodes.reserve(m_material_construction_tasks.size());
    for (std::unique_ptr<MaterialAssemblyTask>& e : m_material_construction_tasks)
    {
        root_nodes.insert(ROOT_NODE_CAST(e.get()));
    }
    m_material_construction_task_graph =
        std::make_unique<core::concurrency::TaskGraph>(
            root_nodes,
            m_globals.get<core::GlobalSettings>()->getNumberOfWorkers(),
            "MaterialConstructionTaskGraph"
        );
    m_material_construction_task_sink = std::make_unique<core::concurrency::TaskSink>(*m_material_construction_task_graph, "MaterialConstruction");
    m_material_construction_task_sink->start();
    m_material_construction_task_sink->submit(0);
}

bool Scene::loadCameras(tinygltf::Model const& model, std::unordered_map<int, int>& camera_ids)
{
    for (auto& [camera_id, camera_id_in_scene] : camera_ids)
    {
        tinygltf::Camera const& camera = model.cameras[camera_id];
        Camera sceneCamera{ camera.name };
        ProjectionType cameraProjectionType{};
        if (camera.type == "perspective")
        {
            cameraProjectionType = ProjectionType::Perspective;
            const tinygltf::PerspectiveCamera& perspectiveCamera = camera.perspective;
            sceneCamera.setPerspective(
                static_cast<float>(perspectiveCamera.yfov), 
                static_cast<float>(perspectiveCamera.aspectRatio),
                static_cast<float>(perspectiveCamera.znear), 
                static_cast<float>(perspectiveCamera.zfar),
                m_global_settings.isInverseDepthClipSpaceEnabled()
            );
        }
        else if (camera.type == "orthographic")
        {
            cameraProjectionType = ProjectionType::Orthographic;
            const tinygltf::OrthographicCamera& orthographicCamera = camera.orthographic;
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

bool Scene::loadAnimations(tinygltf::Model const& model, std::unordered_map<int, int>& animation_ids)
{
    return true;
}

}


