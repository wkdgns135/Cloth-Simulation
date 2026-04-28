#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "engine/components/SimulationComponent.h"

class Cloth;

class PBDClothSimulationComponent final : public SimulationComponent
{
public:
	explicit PBDClothSimulationComponent(Cloth& cloth);

	void start() override;
	void update_simulation(float delta_time) override;

private:
	struct DistanceConstraint
	{
		int particle_a = 0;
		int particle_b = 0;
		float rest_length = 0.0f;
	};

	struct BendingConstraint
	{
		int particle_1 = 0;
		int particle_2 = 0;
		int particle_3 = 0;
		int particle_4 = 0;
		float rest_angle = 0.0f;
	};

	void integrate(float delta_time);
	void rebuild_constraints();
	void rebuild_constraints_if_needed();
	void solve_distance_constraints(float stiffness_per_iteration);
	void solve_distance_constraint(const DistanceConstraint& constraint, float stiffness_per_iteration);
	void solve_bending_constraints(float stiffness_per_iteration);
	void solve_bending_constraint(const BendingConstraint& constraint, float stiffness_per_iteration);
	float per_iteration_stiffness(float stiffness, int iteration_count) const;

	Cloth& cloth_;
	std::vector<DistanceConstraint> distance_constraints_;
	std::vector<BendingConstraint> bending_constraints_;
	glm::vec3 gravity_ = glm::vec3(0.0f, -9.8f, 0.0f);
	float damping_ = 0.99f;
	float stretch_stiffness_ = 1.0f;
	float bend_stiffness_ = 0.15f;
	int constraint_iterations_ = 20;
	std::uint64_t cached_topology_revision_ = 0;
};
