#ifndef LEXGINE_SCENEGRAPH_PBR_MATERIAL_H

#include <memory>
#include <vector>
#include <string>

#include <engine/core/math/vector_types.h>
#include <engine/core/dx/d3d12/tasks/hlsl_compilation_task.h>

#include "lexgine_scenegraph_fwd.h"


namespace lexgine::scenegraph {

class Material
{
public:
    virtual ~Material() = default;
};

struct TextureInfo
{
    std::string textureName;
    std::vector<glm::vec2> uvCoordinates;
    std::unique_ptr<Image> texture;
};


class PbrMaterial : public std::enable_shared_from_this<PbrMaterial>, public Material
{
public:
    // static PbrMaterialHandle create(std::string const& name) { return PbrMaterialHandle { new PbrMaterial { name } }; }
    // static PbrMaterialHandle createDefaultPbrMaterial();

    void setBrdfShader(core::dx::d3d12::tasks::HLSLCompilationTask* shaderCompilationTask);

private:
    PbrMaterial(std::string const& name)
        : m_name{ name }
    {

    }

private:
    std::string m_name;

    // albedo
    TextureInfo m_albedoMap;
    glm::vec4 m_albedoFactor;

    // metallic-roughness
    TextureInfo m_metalnessRoughnessMap;
    float m_roughnessFactor;
    float m_metalnessFactor;

    // normal map
    TextureInfo m_normalMap;
    glm::vec2 m_normalScale;

    // occlusion map
    TextureInfo m_occlusionMap;
    float m_occlusionStrength;
};


}

#endif