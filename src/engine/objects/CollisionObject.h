#pragma once

#include <glm/glm.hpp>

#include "engine/core/WorldObject.h"

class CollisionObject : public WorldObject
{
public:
	~CollisionObject() override = default;
	int update_order() const override { return -100; }

	virtual bool resolve_particle_collision(
		glm::vec3& world_position,
		glm::vec3& world_prev_position,
		float margin) const = 0;
};
