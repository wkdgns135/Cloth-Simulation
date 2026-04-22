#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transform
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	glm::mat4 matrix() const
	{
		glm::mat4 result(1.0f);
		result = glm::translate(result, position);
		result = glm::rotate(result, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		result = glm::rotate(result, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		result = glm::rotate(result, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		result = glm::scale(result, scale);
		return result;
	}
};
