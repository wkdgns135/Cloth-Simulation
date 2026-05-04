#include "engine/components/TransformComponent.h"

#include <cmath>

#include <glm/geometric.hpp>

TransformComponent::TransformComponent()
{
	set_display_name("Transform");
}

glm::mat4 TransformComponent::rotation_matrix() const
{
	glm::mat4 result(1.0f);
	const glm::vec3 euler_rotation = rotation();
	result = glm::rotate(result, euler_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	result = glm::rotate(result, euler_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	result = glm::rotate(result, euler_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
	return result;
}

glm::mat4 TransformComponent::matrix() const
{
	glm::mat4 result(1.0f);
	result = glm::translate(result, position());
	result *= rotation_matrix();
	result = glm::scale(result, scale());
	return result;
}

glm::vec3 TransformComponent::forward() const
{
	return glm::normalize(glm::vec3(rotation_matrix() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
}

glm::vec3 TransformComponent::up() const
{
	return glm::normalize(glm::vec3(rotation_matrix() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
}

glm::vec3 TransformComponent::right() const
{
	return glm::normalize(glm::vec3(rotation_matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
}

void TransformComponent::set_forward(const glm::vec3& direction)
{
	const float direction_length_squared = glm::dot(direction, direction);
	if (direction_length_squared <= 0.000001f)
	{
		return;
	}

	const glm::vec3 normalized_direction = glm::normalize(direction);
	glm::vec3 next_rotation = rotation();
	next_rotation.x = std::asin(glm::clamp(normalized_direction.y, -1.0f, 1.0f));
	next_rotation.y = std::atan2(-normalized_direction.x, -normalized_direction.z);
	next_rotation.z = 0.0f;
	set_rotation(next_rotation);
}

void TransformComponent::look_at(const glm::vec3& target)
{
	set_forward(target - position());
}
