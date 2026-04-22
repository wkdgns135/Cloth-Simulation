#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "engine/simulation/SimulationComponent.h"

class Cloth;
struct Particle;

class ClothSimulationComponent : public SimulationComponent
{
public:
	explicit ClothSimulationComponent(Cloth& cloth);
	~ClothSimulationComponent() override = default;

	void start() override;
	void update_simulation(float delta_time) override final;

protected:
	struct DistanceConstraint
	{
		int particle_a = 0;
		int particle_b = 0;
		float rest_length = 0.0f;
		float compliance = 0.0f;
		float lambda = 0.0f;
	};

	struct DistanceConstraintSolveData
	{
		Particle& particle_a;
		Particle& particle_b;
		glm::vec3 gradient;
		float constraint_value = 0.0f;
		float inverse_mass_a = 0.0f;
		float inverse_mass_b = 0.0f;
		float inverse_mass_sum = 0.0f;
	};

	Cloth& cloth() { return cloth_; }
	const Cloth& cloth() const { return cloth_; }

	void reset_constraint_lambdas();

	virtual void prepare_distance_constraints(float delta_time);
	virtual float compute_distance_delta_lambda(
		DistanceConstraint& constraint,
		const DistanceConstraintSolveData& solve_data,
		float delta_time) = 0;

	float stretch_compliance_ = 0.0f;
	int constraint_iterations_ = 20;

private:
	void integrate(float delta_time);
	void rebuild_constraints();
	void rebuild_constraints_if_needed();
	void solve_distance_constraints(float delta_time);
	void solve_distance_constraint(DistanceConstraint& constraint, float delta_time);

	Cloth& cloth_;
	std::vector<DistanceConstraint> distance_constraints_;
	glm::vec3 gravity_ = glm::vec3(0.0f, -9.8f, 1.0f);
	float damping_ = 0.99f;
	int cached_width_ = 0;
	int cached_height_ = 0;
	float cached_spacing_ = 0.0f;
};
