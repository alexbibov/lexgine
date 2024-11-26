
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>
#undef TINYGLTF_IMPLEMENTATION
#undef TINYGLTF_NO_STB_IMAGE_WRITE

#include <3rd_party/glm/gtc/constants.hpp>
#include <engine/core/globals.h>
#include <engine/core/misc/misc.h>
#include <engine/conversion/image_loader_pool.h>
#include "scene.h"

namespace lexgine::scenegraph
{


std::shared_ptr<Scene> Scene::loadScene(core::Globals& globals, std::filesystem::path const& path_to_scene, int scene_index /* = -1 */)
{
    std::string error_buffer{};
    std::string warning_buffer{};

    tinygltf::TinyGLTF gltf_loader{};
    std::string gltf_path_to_file = path_to_scene.string();
    core::misc::DateTime timestamp{};
    {
        // Fetch the time stamp to be used be the relevant components of the scene: this shall be the update time of the source
        // file containing the scene. Otherwise, if not available for any reason, the timestamp of the build time of the translation unit
        // is being used
        auto gltf_timestamp = core::misc::getFileLastUpdatedTimeStamp(gltf_path_to_file);
        timestamp = gltf_timestamp.isValid() ? *gltf_timestamp : core::misc::DateTime::buildTime();
    }

    tinygltf::Model gltf_model{};

    bool result = gltf_loader.LoadASCIIFromFile(&gltf_model, &error_buffer, &warning_buffer, gltf_path_to_file, tinygltf::SectionCheck::REQUIRE_VERSION);
    auto rv = std::shared_ptr<Scene>{ new Scene() };
    rv->m_scene_path = path_to_scene;


    if (!result) {
        LEXGINE_LOG_ERROR(*rv, "Unable to load gltf file '" + gltf_path_to_file + "'");
        return nullptr;
    }
    if (!error_buffer.empty())
    {
        LEXGINE_LOG_ERROR(*rv, "Error loading gltf file '" + gltf_path_to_file + "': " + error_buffer);
        return nullptr;
    }

    if (!warning_buffer.empty())
    {
        core::misc::Log::retrieve()->out("Warning loading gltf file '" + gltf_path_to_file + "': " + warning_buffer, core::misc::LogMessageType::exclamation);
    }


    rv->setStringName("gltf_scene_" + path_to_scene.stem().string());

    // Check used extensions
    for (auto& ext : gltf_model.extensionsUsed)
    {
        auto it = rv->m_enabled_extensions.find(ext);
        if (it == rv->m_enabled_extensions.end())
        {
            if (std::find(gltf_model.extensionsRequired.begin(), gltf_model.extensionsRequired.end(), ext) != gltf_model.extensionsRequired.end())
            {
                LEXGINE_LOG_ERROR(*rv, "Unable to load gltf file '" + gltf_path_to_file + "': required extension " + ext.c_str() + " is not supported");
                return nullptr;
            }
            else {
                core::misc::Log::retrieve()->out("gltf file '" + gltf_path_to_file + "' contains unsupported extension " + ext.c_str(), core::misc::LogMessageType::exclamation);
            }
        }
        else
        {
            it->second = true;
        }
    }

    // Prepare scene memory
    {
        uint64_t scene_memory_size = std::accumulate(gltf_model.buffers.begin(), gltf_model.buffers.end(),
            static_cast<uint64_t>(0),
            [](uint64_t acc, tinygltf::Buffer const& e)
            {
                return acc + static_cast<uint64_t>(e.data.size());
            }
        );

        rv->m_scene_memory.scene_memory_buffer.reset(new SceneMeshMemory{ globals, scene_memory_size });
        for (tinygltf::Buffer const& buffer : gltf_model.buffers)
        {
            rv->m_scene_memory.m_scene_memory_handles.push_back(
                rv->m_scene_memory.scene_memory_buffer->addData(buffer.data.data(),
                    buffer.data.size())
            );
        }

    }
        
    // Parse scene nodes and construct scene graph
    //for (tinygltf::Node& node : gltf_model.nodes)
    //{
    //    if()
    //    if (mesh.extensions.find(c_ext_mesh_gpu_instancing) != mesh.extensions.end())
    //    {
    //        // Mesh has GPU instancing
    //        tinygltf::Value const& ext_mesh_gpu_instancing = mesh.extensions.at(c_ext_mesh_gpu_instancing);

    //        for (std::string const& e : ext_mesh_gpu_instancing.Keys())
    //        {

    //        }
    //    }
    //}

    if (!rv->loadLights(gltf_model)) return nullptr;
    if (!rv->loadTextures(gltf_model, timestamp, *globals.get<conversion::ImageLoaderPool>())) return nullptr;
    if (!rv->loadMaterials(gltf_model)) return nullptr;
    if (!rv->loadMeshes(gltf_model)) return nullptr;
    /*if (!rv->loadCameras(gltf_model)) return nullptr;
    if (!rv->loadNodes(gltf_model)) return nullptr;
    if (!rv->loadAnimations(gltf_model)) return nullptr;*/

    return rv;
}

bool Scene::loadLights(tinygltf::Model& model)
{
    if (m_enabled_extensions[c_khr_light_punctual_ext])
    {
        if (model.extensions.find(c_khr_light_punctual_ext) == model.extensions.end()
            || !model.extensions.at(c_khr_light_punctual_ext).Has("lights"))
        {
            return false;
        }

        auto& khrLights = model.extensions.at(c_khr_light_punctual_ext).Get("lights");
        m_lights.clear();
        m_lights.reserve(khrLights.ArrayLen());
        for (size_t i = 0; i < khrLights.ArrayLen(); ++i)
        {
            auto& light = khrLights.Get(static_cast<int>(i));

            // Retrieve light type
            if (!light.Has("type")) {
                LEXGINE_LOG_ERROR(this, std::string{ c_khr_light_punctual_ext } + ": light " + std::to_string(i) + " does not have a type");
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
                    LEXGINE_LOG_ERROR(this, std::string{ c_khr_light_punctual_ext } + ": light " + std::to_string(i) + " has invalid type");
                    return false;
                }
            }

            Light lexgineLight{ lightType };
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
                    LEXGINE_LOG_ERROR(this, std::string{ c_khr_light_punctual_ext } + ": invalid description of spot light " + std::to_string(i) + ", the light does not define a 'spot' property, which is required");
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
            }

            case LightType::point:
                lexgineLight.setRange(static_cast<float>(light.Get("range").Get<double>()));
                break;

            }

            m_lights.push_back(lexgineLight);
        }
    }

    return true;
}

bool Scene::loadTextures(tinygltf::Model& model, core::misc::Optional<core::misc::DateTime> const& timestamp, conversion::ImageLoaderPool const& image_loader_pool)
{
    size_t const num_images = model.images.size();
    m_images.reserve(num_images);

    for (size_t i = 0; i < num_images; ++i)
    {
        auto& gltf_image = model.images.at(i);
        if (gltf_image.image.empty())
        {
            // image is loaded from uri
            m_images.emplace_back(m_scene_path / gltf_image.uri, image_loader_pool);
        }
        else
        {
            // image is embedded in gltf
            m_images.emplace_back(std::move(gltf_image.image), static_cast<uint32_t>(gltf_image.width), static_cast<uint32_t>(gltf_image.height),
                static_cast<size_t>(gltf_image.component), static_cast<size_t>(gltf_image.bits), conversion::ImageColorSpace::srgb, gltf_image.name, *timestamp, image_loader_pool);
        }
    }

    return true;
}

bool Scene::loadMaterials(tinygltf::Model& model)
{
    return true;
}

bool Scene::loadMeshes(tinygltf::Model& model)
{
    // Parse meshes
    for (tinygltf::Mesh const& mesh : model.meshes)
    {
        // Parse mesh primitives
        auto const& morph_weights = mesh.weights;
        for (tinygltf::Primitive const& mesh_primitive : mesh.primitives)
        {
            lexgine::core::VertexAttributeSpecificationList vertex_attributes {};
            for (auto const& e : mesh_primitive.attributes) 
            {
                std::string const& attribute_name = e.first;
                int accessor_id = e.second;
            }
        }
    }
    return true;
}

}
