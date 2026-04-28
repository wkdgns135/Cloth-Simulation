#pragma once

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transform
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	glm::mat4 rotation_matrix() const
	{
		glm::mat4 result(1.0f);
		result = glm::rotate(result, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		result = glm::rotate(result, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		result = glm::rotate(result, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		return result;
	}

	glm::mat4 matrix() const
	{
		glm::mat4 result(1.0f);
		result = glm::translate(result, position);
		result *= rotation_matrix();
		result = glm::scale(result, scale);
		return result;
	}

	glm::vec3 forward() const
	{
		return glm::normalize(glm::vec3(rotation_matrix() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
	}

	glm::vec3 up() const
	{
		return glm::normalize(glm::vec3(rotation_matrix() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
	}

	glm::vec3 right() const
	{
		return glm::normalize(glm::vec3(rotation_matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
	}

	void set_forward(const glm::vec3& direction)
	{
		const float direction_length_squared = glm::dot(direction, direction);
		if (direction_length_squared <= 0.000001f)
		{
			return;
		}

		const glm::vec3 normalized_direction = glm::normalize(direction);
		rotation.x = std::asin(glm::clamp(normalized_direction.y, -1.0f, 1.0f));
		rotation.y = std::atan2(-normalized_direction.x, -normalized_direction.z);
		rotation.z = 0.0f;
	}

	void look_at(const glm::vec3& target)
	{
		set_forward(target - position);
	}
};
