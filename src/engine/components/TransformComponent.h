#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/Component.h"

class TransformComponent final : public Component
{
public:
	TransformComponent();

	PROPERTY(glm::vec3, position, "Transform", "Position", glm::vec3(0.0f))
	PROPERTY(glm::vec3, rotation, "Transform", "Rotation", glm::vec3(0.0f))
	PROPERTY(glm::vec3, scale, "Transform", "Scale", glm::vec3(1.0f))

	glm::mat4 rotation_matrix() const;
	glm::mat4 matrix() const;
	glm::vec3 forward() const;
	glm::vec3 up() const;
	glm::vec3 right() const;
	void set_forward(const glm::vec3& direction);
	void look_at(const glm::vec3& target);
};
