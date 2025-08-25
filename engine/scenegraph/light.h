#ifndef LEXGINE_SCENEGRAPH_LIGHT_H
#define LEXGINE_SCENEGRAPH_LIGHT_H

#include <optional>

#include <glm/glm.hpp>
#include <engine/core/entity.h>
#include "class_names.h"

namespace lexgine::scenegraph {

enum class LightType
{
    directional,
    point,
    spot,
    count
};

class Light : public core::NamedEntity<class_names::Light>
{
public:
    Light(LightType type) : m_type{ type } {}

    LightType type() const { return m_type; }

    std::string lightTypeString() const;

    void setColor(glm::vec3 const& color) { m_color = color; }
    glm::vec3 getColor() const { return m_color; }

    void setDirection(glm::vec3 const& direction);
    std::optional<glm::vec3> getDirection() const;

    void setIntensity(float intensity) { m_intensity = intensity; }
    float getIntensity() const { return m_intensity; }

    void setRange(float range);
    std::optional<float> getRange() const;


    void setInnerConeAngle(float inner_cone_angle);
    std::optional<float> getInnerConeAngle() const;

    void setOuterConeAngle(float outer_cone_angle);
    std::optional<float> getOuterConeAngle() const;

private:
    LightType m_type;
    glm::vec3 m_color;
    glm::vec3 m_direction;
    float m_intensity;
    float m_range;
    float m_inner_cone_angle;
    float m_outer_cone_angle;
};

}

#endif
