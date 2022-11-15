#include "light.h"

namespace lexgine::scenegraph
{

std::string Light::lightTypeString() const
{
    switch (m_type)
    {
    case LightType::directional:
        return "directional";
    case LightType::point:
        return "point";
    case LightType::spot:
        return "spot";
    }

    return "unknown";
}

void Light::setDirection(glm::vec3 const& direction)
{
    if (m_type == LightType::point)
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to assign direction to a point light, which doesn't support it", core::misc::LogMessageType::exclamation);
    }
    else
    {
        m_direction = direction;
    }
}

std::optional<glm::vec3> Light::getDirection() const
{
    if (m_type == LightType::point)
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to retrieve direction of a point light, which doesn't support it", core::misc::LogMessageType::exclamation);
        return std::nullopt;
    }
    else
    {
        return m_direction;
    }
}

void Light::setRange(float range)
{
    if (m_type == LightType::directional)
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to set range for a directional light, which doesn't support it", core::misc::LogMessageType::exclamation);
    }
    else
    {
        m_range = range;
    }
}

std::optional<float> Light::getRange() const
{
    if (m_type == LightType::directional)
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to retrieve the range of a directional light, which doesn't support it", core::misc::LogMessageType::exclamation);
        return std::nullopt;
    }
    else
    {
        return m_range;
    }
}

void Light::setInnerConeAngle(float inner_cone_angle)
{
    if (m_type == LightType::spot)
    {
        m_inner_cone_angle = inner_cone_angle;
    }
    else
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to assign inner cone angle value to a " + lightTypeString() + " light, which doesn't support it", core::misc::LogMessageType::exclamation);
    }
}

std::optional<float> Light::getInnerConeAngle() const
{
    if (m_type == LightType::spot)
    {
        return m_inner_cone_angle;
    }
    else
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to retrieve inner cone angle value from a " + lightTypeString() + " light, which doesn't support it", core::misc::LogMessageType::exclamation);
        return std::nullopt;
    }
}

void Light::setOuterConeAngle(float outer_cone_angle)
{
    if (m_type == LightType::spot)
    {
        m_outer_cone_angle = outer_cone_angle;
    }
    else
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to assign outer cone angle value to a " + lightTypeString() + " light, which doesn't support it", core::misc::LogMessageType::exclamation);
    }
}

std::optional<float> Light::getOuterConeAngle() const
{
    if (m_type == LightType::spot)
    {
        return m_outer_cone_angle;
    }
    else
    {
        core::misc::Log::retrieve()->out("WARNING: attempted to retrieve outer cone angle value from a " + lightTypeString() + " light, which doesn't support it", core::misc::LogMessageType::exclamation);
        return std::nullopt;
    }
}

}
