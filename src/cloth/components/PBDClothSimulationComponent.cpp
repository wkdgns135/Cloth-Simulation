#include "cloth/components/PBDClothSimulationComponent.h"

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"

PBDClothSimulationComponent::PBDClothSimulationComponent(Cloth& cloth)
	: ClothSimulationComponentBase(cloth)
{
}

void PBDClothSimulationComponent::update_simulation(float delta_time)
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	rebuild_constraints_if_needed();
	integrate(delta_time);

	const int iterations = constraint_iterations();
	const float stretch_stiffness = per_iteration_stiffness(stretch_stiffness_);
	const float bend_stiffness = per_iteration_stiffness(bend_stiffness_);

	for (int i = 0; i < iterations; ++i)
	{
		solve_distance_constraints(stretch_stiffness);
		solve_bending_constraints(bend_stiffness);
	}
}

void PBDClothSimulationComponent::solve_distance_constraints(float stiffness_per_iteration)
{
	for (const DistanceConstraint& constraint : distance_constraints())
	{
		solve_distance_constraint(constraint, stiffness_per_iteration);
	}
}

void PBDClothSimulationComponent::solve_distance_constraint(
	const DistanceConstraint& constraint,
	float stiffness_per_iteration)
{
	if (stiffness_per_iteration <= 0.0f)
	{
		return;
	}

	DistanceConstraintEvaluation evaluation;
	if (!evaluate_distance_constraint(constraint, evaluation))
	{
		return;
	}

	auto& particles = cloth().get_particles();
	Particle& particle_a = particles[constraint.particle_a];
	Particle& particle_b = particles[constraint.particle_b];

	const float delta_lambda = -stiffness_per_iteration * evaluation.constraint_value / evaluation.denominator;
	const glm::vec3 correction = delta_lambda * evaluation.gradient;

	particle_a.position += evaluation.inverse_mass_a * correction;
	particle_b.position -= evaluation.inverse_mass_b * correction;
}

void PBDClothSimulationComponent::solve_bending_constraints(float stiffness_per_iteration)
{
	for (const BendingConstraint& constraint : bending_constraints())
	{
		solve_bending_constraint(constraint, stiffness_per_iteration);
	}
}

void PBDClothSimulationComponent::solve_bending_constraint(
	const BendingConstraint& constraint,
	float stiffness_per_iteration)
{
	if (stiffness_per_iteration <= 0.0f)
	{
		return;
	}

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

	const float scale = -stiffness_per_iteration * evaluation.constraint_value / evaluation.denominator;

	particle_1.position += evaluation.inverse_mass_1 * scale * evaluation.q1;
	particle_2.position += evaluation.inverse_mass_2 * scale * evaluation.q2;
	particle_3.position += evaluation.inverse_mass_3 * scale * evaluation.q3;
	particle_4.position += evaluation.inverse_mass_4 * scale * evaluation.q4;
}
