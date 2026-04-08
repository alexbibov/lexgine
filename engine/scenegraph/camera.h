#ifndef LEXGINE_SCENEGRAPH_CAMERA_H
#define LEXGINE_SCENEGRAPH_CAMERA_H

#include <array>
#include <glm/glm.hpp>
#include "engine/core/math/matrix_types.h"
#include "engine/core/math/vector_types.h"

namespace lexgine::scenegraph
{


enum class ProjectionType {
	Perspective,
	Orthographic
};

class Camera {
public:
	Camera(std::string const& name);

	void setPerspective(float fov_y_degrees, float aspect, float near_z, float far_z, bool invert_depth = true);
	void setOrthographic(float left, float right, float bottom, float top, float near_z, float far_z);
	void setView(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);

	glm::mat4 getViewMatrix() const;
	glm::mat4 getProjectionMatrix() const;
	glm::mat4 getViewProjectionMatrix() const;
	ProjectionType getProjectionType() const { return m_projection_type; }

	bool isAabbVisible(const glm::vec3& min, const glm::vec3& max);

private:
	void updateFrustumPlanes();

private:
	std::string m_name;
	ProjectionType m_projection_type;

	lexgine::core::math::Matrix4f m_view_matrix;
	lexgine::core::math::Matrix4f m_projection_matrix;
	lexgine::core::math::Matrix4f m_view_projection_matrix;

	lexgine::core::math::Vector3f m_position;
	lexgine::core::math::Vector3f m_forward;
	lexgine::core::math::Vector3f m_up;

	std::array<glm::vec4, 6> m_frustum_planes;
};

}

#endif