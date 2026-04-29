#pragma once

#include <algorithm>
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

	void set_gravity(const glm::vec3& gravity) { gravity_ = gravity; }
	const glm::vec3& gravity() const { return gravity_; }

	void set_external_acceleration(const glm::vec3& acceleration) { external_acceleration_ = acceleration; }
	void add_external_acceleration(const glm::vec3& acceleration) { external_acceleration_ += acceleration; }
	const glm::vec3& external_acceleration() const { return external_acceleration_; }

	void set_damping(float damping) { damping_ = std::clamp(damping, 0.0f, 1.0f); }
	float damping() const { return damping_; }

	void set_constraint_iterations(int iterations) { constraint_iterations_ = std::max(0, iterations); }
	int constraint_iterations() const { return constraint_iterations_; }

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

private:
	Cloth& cloth_;
	std::vector<DistanceConstraint> distance_constraints_;
	std::vector<BendingConstraint> bending_constraints_;
	glm::vec3 gravity_ = glm::vec3(0.0f, -9.8f, 0.0f);
	glm::vec3 external_acceleration_ = glm::vec3(0.0f);
	float damping_ = 0.99f;
	int constraint_iterations_ = 20;
	std::uint64_t cached_topology_revision_ = 0;
};
