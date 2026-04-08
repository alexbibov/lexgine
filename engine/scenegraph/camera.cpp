#include "camera.h"
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "engine/core/math/utility.h"

namespace lexgine::scenegraph
{ 

Camera::Camera(std::string const& name)
	: m_name{ name }
	, m_projection_type{ ProjectionType::Perspective }
	, m_position{ 0.0f }
	, m_forward{ 0.0f, 0.0f, -1.0f }
	, m_up{ 0.0f, 1.0f, 0.0f }
{
	setPerspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	setView(m_position, m_position + m_forward, m_up);
}

void Camera::setPerspective(float fov_y_degrees, float aspect, float near_z, float far_z, bool invert_depth/* = true*/) {
	m_projection_type = ProjectionType::Perspective;
	m_projection_matrix = core::math::createProjectionMatrix(aspect, fov_y_degrees, near_z, far_z, invert_depth);
	updateFrustumPlanes();
}

void Camera::setOrthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
	m_projection_type = ProjectionType::Orthographic;
	m_projection_matrix = core::math::createOrthogonalProjectionMatrix(left, top, right - left, bottom - top, near_z, far_z);
	updateFrustumPlanes();
}

void Camera::setView(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up_vec) {
	m_position = pos;
	m_forward = glm::normalize(target - pos);
	m_up = glm::normalize(up_vec);
	m_view_matrix = glm::lookAt(m_position, target, m_up);
	updateFrustumPlanes();
}

glm::mat4 Camera::getViewMatrix() const {
	return m_view_matrix;
}

glm::mat4 Camera::getProjectionMatrix() const {
	return m_projection_matrix;
}

glm::mat4 Camera::getViewProjectionMatrix() const {
	return m_view_projection_matrix;
}

void Camera::updateFrustumPlanes() {
	m_view_projection_matrix = m_projection_matrix * m_view_matrix;
	const glm::mat4& m = m_view_projection_matrix;

	m_frustum_planes[0] = glm::vec4(m[0][3] + m[0][0],
		m[1][3] + m[1][0],
		m[2][3] + m[2][0],
		m[3][3] + m[3][0]);

	m_frustum_planes[1] = glm::vec4(m[0][3] - m[0][0],
		m[1][3] - m[1][0],
		m[2][3] - m[2][0],
		m[3][3] - m[3][0]);

	m_frustum_planes[2] = glm::vec4(m[0][3] + m[0][1],
		m[1][3] + m[1][1],
		m[2][3] + m[2][1],
		m[3][3] + m[3][1]);

	m_frustum_planes[3] = glm::vec4(m[0][3] - m[0][1],
		m[1][3] - m[1][1],
		m[2][3] - m[2][1],
		m[3][3] - m[3][1]);

	m_frustum_planes[4] = glm::vec4(m[0][3] + m[0][2],
		m[1][3] + m[1][2],
		m[2][3] + m[2][2],
		m[3][3] + m[3][2]);

	m_frustum_planes[5] = glm::vec4(m[0][3] - m[0][2],
		m[1][3] - m[1][2],
		m[2][3] - m[2][2],
		m[3][3] - m[3][2]);

	for (auto& plane : m_frustum_planes) {
		float length = glm::length(glm::vec3(plane));
		plane /= length;
	}
}

bool Camera::isAabbVisible(const glm::vec3& min, const glm::vec3& max) {
	for (const auto& plane : m_frustum_planes) {
		glm::vec3 positive_vertex = min;

		if (plane.x >= 0) positive_vertex.x = max.x;
		if (plane.y >= 0) positive_vertex.y = max.y;
		if (plane.z >= 0) positive_vertex.z = max.z;

		if (glm::dot(glm::vec3(plane), positive_vertex) + plane.w < 0)
			return false;
	}
	return true;
}


}