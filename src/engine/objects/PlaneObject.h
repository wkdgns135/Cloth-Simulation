#pragma once

#include "engine/objects/CollisionObject.h"

class PlaneObject final : public CollisionObject
{
public:
	PlaneObject();

	bool resolve_particle_collision(
		glm::vec3& world_position,
		glm::vec3& world_prev_position,
		float margin) const override;
};
