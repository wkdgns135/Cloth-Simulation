#pragma once

#include <glm/glm.hpp>

struct Particle
{
	glm::vec3 position;
	glm::vec3 prev_position;
	bool is_fixed = false;

	Particle() = default;

	Particle(const glm::vec3& pos)
		: position(pos), prev_position(pos)
	{
	}
};
