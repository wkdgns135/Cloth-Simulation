#pragma once

#include "engine/objects/CollisionObject.h"

class SphereObject final : public CollisionObject
{
public:
	explicit SphereObject(float radius = 1.0f);

	PROPERTY(glm::vec3, linear_velocity, "Motion", "Linear Velocity", glm::vec3(0.0f))
	PROPERTY_RANGE(float, max_travel_distance, "Motion", "Max Travel Distance", 0.0f, 0.0f, 1000.0f, 0.1f)
	PROPERTY(bool, lifetime_limit_enabled, "Motion", "Lifetime Limit Enabled", false)
	void configure_projectile(const glm::vec3& velocity, float travel_distance_limit);

	void update(float delta_time) override;

	bool hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const override;
	bool resolve_particle_collision(
		glm::vec3& world_position,
		glm::vec3& world_prev_position,
		float margin) const override;

	float radius() const { return radius_; }

private:
	float world_radius() const;

	float radius_ = 1.0f;
	glm::vec3 spawn_position_ = glm::vec3(0.0f);
};
