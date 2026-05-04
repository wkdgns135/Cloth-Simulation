#pragma once

#include "engine/objects/CollisionObject.h"

class PlaneObject final : public CollisionObject
{
public:
	PlaneObject();

	bool hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const override;
	bool resolve_particle_collision(
		glm::vec3& world_position,
		glm::vec3& world_prev_position,
		float margin) const override;
};
