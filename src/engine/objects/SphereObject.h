#pragma once

#include <algorithm>

#include "engine/objects/CollisionObject.h"

class SphereObject final : public CollisionObject
{
public:
	explicit SphereObject(float radius = 1.0f);

	void set_linear_velocity(const glm::vec3& velocity) { linear_velocity_ = velocity; }
	const glm::vec3& linear_velocity() const { return linear_velocity_; }
	void configure_projectile(const glm::vec3& velocity, float max_travel_distance);

	void update(float delta_time) override;

	bool resolve_particle_collision(
		glm::vec3& world_position,
		glm::vec3& world_prev_position,
		float margin) const override;

	float radius() const { return radius_; }

private:
	float world_radius() const;

	float radius_ = 1.0f;
	glm::vec3 linear_velocity_ = glm::vec3(0.0f);
	glm::vec3 spawn_position_ = glm::vec3(0.0f);
	float max_travel_distance_ = 0.0f;
	bool lifetime_limit_enabled_ = false;
};
