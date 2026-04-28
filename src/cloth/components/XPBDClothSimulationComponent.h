#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "engine/simulation/SimulationComponent.h"

class Cloth;

class XPBDClothSimulationComponent final : public SimulationComponent
{
public:
	explicit XPBDClothSimulationComponent(Cloth& cloth);

	void start() override;
	void update_simulation(float delta_time) override;

private:
	struct DistanceConstraint
	{
		int particle_a = 0;
		int particle_b = 0;
		float rest_length = 0.0f;
		float compliance = 0.0f;
		float lambda = 0.0f;
	};

	void integrate(float delta_time);
	void rebuild_constraints();
	void rebuild_constraints_if_needed();
	void reset_constraint_lambdas();
	void solve_distance_constraints(float delta_time);
	void solve_distance_constraint(DistanceConstraint& constraint, float delta_time);

	Cloth& cloth_;
	std::vector<DistanceConstraint> distance_constraints_;
	glm::vec3 gravity_ = glm::vec3(0.0f, -9.8f, 0.0f);
	float damping_ = 0.99f;
	float stretch_compliance_ = 0.000001f;
	float bend_compliance_ = 0.0005f;
	int constraint_iterations_ = 20;
	std::uint64_t cached_topology_revision_ = 0;
};
