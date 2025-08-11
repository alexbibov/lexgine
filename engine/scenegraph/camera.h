#ifndef LEXGINE_SCENEGRAPH_CAMERA_H
#define LEXGINE_SCENEGRAPH_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace lexgine::scenegraph
{


enum class ProjectionType {
	Perspective,
	Orthographic
};

class Camera {
public:
	Camera();

	void setPerspective(float fov_y_degrees, float aspect, float near_z, float far_z);
	void setOrthographic(float left, float right, float bottom, float top, float near_z, float far_z);
	void setView(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);

	glm::mat4 getViewMatrix() const;
	glm::mat4 getProjectionMatrix() const;
	glm::mat4 getViewProjectionMatrix() const;

	bool isAabbVisible(const glm::vec3& min, const glm::vec3& max);

private:
	void updateFrustumPlanes();

	ProjectionType projection_type;

	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;
	glm::mat4 view_projection_matrix;

	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 up;

	std::array<glm::vec4, 6> frustum_planes;
};

}

#endif