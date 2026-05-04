#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "engine/components/SimulationComponent.h"

class Cloth;

class ClothSimulationComponentBase : public SimulationComponent
{
public:
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
		std::array<float, 16> q_matrix = {};
	};

	struct DistanceConstraintEvaluation
	{
		glm::vec3 gradient = glm::vec3(0.0f);
		float constraint_value = 0.0f;
		float inverse_mass_a = 0.0f;
		float inverse_mass_b = 0.0f;
		float denominator = 0.0f;
	};

	struct BendingConstraintEvaluation
	{
		glm::vec3 q1 = glm::vec3(0.0f);
		glm::vec3 q2 = glm::vec3(0.0f);
		glm::vec3 q3 = glm::vec3(0.0f);
		glm::vec3 q4 = glm::vec3(0.0f);
		float constraint_value = 0.0f;
		float inverse_mass_1 = 0.0f;
		float inverse_mass_2 = 0.0f;
		float inverse_mass_3 = 0.0f;
		float inverse_mass_4 = 0.0f;
		float denominator = 0.0f;
	};

	explicit ClothSimulationComponentBase(Cloth& cloth);

	void start() override;
	void set_external_acceleration(const glm::vec3& acceleration) { external_acceleration_ = acceleration; }
	void add_external_acceleration(const glm::vec3& acceleration) { external_acceleration_ += acceleration; }
	const glm::vec3& external_acceleration() const { return external_acceleration_; }

	PROPERTY(glm::vec3, gravity, "Simulation", "Gravity", glm::vec3(0.0f, -9.8f, 0.0f))
	PROPERTY_RANGE(float, damping, "Simulation", "Damping", 0.99f, 0.0f, 1.0f, 0.01f)
	PROPERTY_RANGE(int, constraint_iterations, "Simulation", "Constraint Iterations", 20, 1, 128, 1)
	PROPERTY(bool, collision_enabled, "Simulation", "Collision Enabled", true)
	PROPERTY_RANGE(float, collision_margin, "Simulation", "Collision Margin", 0.01f, 0.0f, 1.0f, 0.001f)
	
protected:
	Cloth& cloth() { return cloth_; }
	const Cloth& cloth() const { return cloth_; }

	void integrate(float delta_time);
	bool rebuild_constraints();
	bool rebuild_constraints_if_needed();

	const std::vector<DistanceConstraint>& distance_constraints() const { return distance_constraints_; }
	const std::vector<BendingConstraint>& bending_constraints() const { return bending_constraints_; }

	bool evaluate_distance_constraint(
		const DistanceConstraint& constraint,
		DistanceConstraintEvaluation& evaluation) const;
	bool evaluate_bending_constraint(
		const BendingConstraint& constraint,
		BendingConstraintEvaluation& evaluation) const;

	float per_iteration_stiffness(float stiffness) const;
	void solve_collision_objects();

private:
	glm::vec3 world_to_local_point(const glm::vec3& world_point) const;
	glm::vec3 local_to_world_point(const glm::vec3& local_point) const;

	Cloth& cloth_;
	std::vector<DistanceConstraint> distance_constraints_;
	std::vector<BendingConstraint> bending_constraints_;
	glm::vec3 external_acceleration_ = glm::vec3(0.0f);
	std::uint64_t cached_topology_revision_ = 0;
};
