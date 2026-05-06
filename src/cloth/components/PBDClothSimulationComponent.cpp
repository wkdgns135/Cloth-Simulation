#include "cloth/components/PBDClothSimulationComponent.h"

#include <algorithm>

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"

PBDClothSimulationComponent::PBDClothSimulationComponent(Cloth& cloth)
	: ClothSimulationComponentBase(cloth)
{
	set_display_name("PBD Solver");
}

void PBDClothSimulationComponent::update_simulation(float delta_time)
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	rebuild_constraints_if_needed();
	
	const int pass_count = solver_pass_count();
	const float substep_delta_time = delta_time / static_cast<float>(pass_count);
	prepare_self_collision_candidates(delta_time);

	for (int i = 0; i < pass_count; ++i)
	{
		integrate(substep_delta_time);
		solve_collision_objects();
		solve_constraints();
		solve_self_collision();
	}

	clear_external_acceleration();
}

float PBDClothSimulationComponent::stiffness_for_constraint(const DistanceConstraint& constraint) const
{
	switch (constraint.kind)
	{
	case DistanceConstraint::Kind::Stretch:
		return per_iteration_stiffness(stretch_stiffness());
	case DistanceConstraint::Kind::Shear:
		return per_iteration_stiffness(shear_stiffness());
	case DistanceConstraint::Kind::Bend:
	default:
		return per_iteration_stiffness(bend_stiffness());
	}
}

void PBDClothSimulationComponent::solve_constraints()
{
	for (const DistanceConstraint& constraint : distance_constraints())
	{
		solve_distance_constraint(constraint);
	}
}

void PBDClothSimulationComponent::solve_distance_constraint(const DistanceConstraint& constraint)
{
	const float stiffness_per_iteration = stiffness_for_constraint(constraint);
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
