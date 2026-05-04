#include "cloth/components/XPBDClothSimulationComponent.h"

#include <algorithm>

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"

XPBDClothSimulationComponent::XPBDClothSimulationComponent(Cloth& cloth)
	: ClothSimulationComponentBase(cloth)
{
	set_display_name("XPBD Solver");
}

void XPBDClothSimulationComponent::start()
{
	ClothSimulationComponentBase::start();
	sync_constraint_states();
}

void XPBDClothSimulationComponent::update_simulation(float delta_time)
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	if (rebuild_constraints_if_needed())
	{
		sync_constraint_states();
	}

	integrate(delta_time);
	reset_constraint_lambdas();

	const int iterations = constraint_iterations();
	const int solver_passes = std::max(1, iterations);
	for (int i = 0; i < solver_passes; ++i)
	{
		if (i < iterations)
		{
			solve_distance_constraints(delta_time);
			solve_bending_constraints(delta_time);
		}

		solve_collision_objects();
	}
}

void XPBDClothSimulationComponent::sync_constraint_states()
{
	distance_constraint_lambdas_.assign(distance_constraints().size(), 0.0f);
	bending_constraint_lambdas_.assign(bending_constraints().size(), 0.0f);
}

void XPBDClothSimulationComponent::reset_constraint_lambdas()
{
	std::fill(distance_constraint_lambdas_.begin(), distance_constraint_lambdas_.end(), 0.0f);
	std::fill(bending_constraint_lambdas_.begin(), bending_constraint_lambdas_.end(), 0.0f);
}

void XPBDClothSimulationComponent::solve_distance_constraints(float delta_time)
{
	for (std::size_t i = 0; i < distance_constraints().size(); ++i)
	{
		solve_distance_constraint(i, delta_time);
	}
}

void XPBDClothSimulationComponent::solve_distance_constraint(std::size_t constraint_index, float delta_time)
{
	const DistanceConstraint& constraint = distance_constraints()[constraint_index];

	DistanceConstraintEvaluation evaluation;
	if (!evaluate_distance_constraint(constraint, evaluation))
	{
		return;
	}

	auto& particles = cloth().get_particles();
	Particle& particle_a = particles[constraint.particle_a];
	Particle& particle_b = particles[constraint.particle_b];

	const float alpha_tilde = stretch_compliance() / (delta_time * delta_time);
	const float denominator = evaluation.denominator + alpha_tilde;
	if (denominator <= 0.0f)
	{
		return;
	}

	const float delta_lambda =
		(-evaluation.constraint_value - alpha_tilde * distance_constraint_lambdas_[constraint_index]) / denominator;
	const glm::vec3 correction = delta_lambda * evaluation.gradient;

	distance_constraint_lambdas_[constraint_index] += delta_lambda;
	particle_a.position += evaluation.inverse_mass_a * correction;
	particle_b.position -= evaluation.inverse_mass_b * correction;
}

void XPBDClothSimulationComponent::solve_bending_constraints(float delta_time)
{
	for (std::size_t i = 0; i < bending_constraints().size(); ++i)
	{
		solve_bending_constraint(i, delta_time);
	}
}

void XPBDClothSimulationComponent::solve_bending_constraint(std::size_t constraint_index, float delta_time)
{
	const BendingConstraint& constraint = bending_constraints()[constraint_index];

	BendingConstraintEvaluation evaluation;
	if (!evaluate_bending_constraint(constraint, evaluation))
	{
		return;
	}

	auto& particles = cloth().get_particles();
	Particle& particle_1 = particles[constraint.particle_1];
	Particle& particle_2 = particles[constraint.particle_2];
	Particle& particle_3 = particles[constraint.particle_3];
	Particle& particle_4 = particles[constraint.particle_4];

	const float alpha_tilde = bend_compliance() / (delta_time * delta_time);
	const float denominator = evaluation.denominator + alpha_tilde;
	if (denominator <= 0.0f)
	{
		return;
	}

	const float delta_lambda =
		(-evaluation.constraint_value - alpha_tilde * bending_constraint_lambdas_[constraint_index]) / denominator;

	bending_constraint_lambdas_[constraint_index] += delta_lambda;
	particle_1.position += evaluation.inverse_mass_1 * delta_lambda * evaluation.q1;
	particle_2.position += evaluation.inverse_mass_2 * delta_lambda * evaluation.q2;
	particle_3.position += evaluation.inverse_mass_3 * delta_lambda * evaluation.q3;
	particle_4.position += evaluation.inverse_mass_4 * delta_lambda * evaluation.q4;
}
