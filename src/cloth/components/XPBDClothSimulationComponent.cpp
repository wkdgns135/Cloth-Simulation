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

	const int pass_count = solver_pass_count();
	const float substep_delta_time = delta_time / static_cast<float>(pass_count);
	prepare_self_collision_candidates(delta_time);

	for (int i = 0; i < pass_count; ++i)
	{
		integrate(substep_delta_time);
		reset_constraint_lambdas();
		solve_collision_objects();
		solve_constraints(substep_delta_time);
		solve_self_collision();
	}

	clear_external_acceleration();
}

float XPBDClothSimulationComponent::compliance_for_constraint(const DistanceConstraint& constraint) const
{
	switch (constraint.kind)
	{
	case DistanceConstraint::Kind::Stretch:
		return stretch_compliance();
	case DistanceConstraint::Kind::Shear:
		return shear_compliance();
	case DistanceConstraint::Kind::Bend:
	default:
		return bend_compliance();
	}
}

void XPBDClothSimulationComponent::sync_constraint_states()
{
	distance_constraint_lambdas_.assign(distance_constraints().size(), 0.0f);
}

void XPBDClothSimulationComponent::reset_constraint_lambdas()
{
	std::fill(distance_constraint_lambdas_.begin(), distance_constraint_lambdas_.end(), 0.0f);
}

void XPBDClothSimulationComponent::solve_constraints(float delta_time)
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

	const float alpha_tilde = compliance_for_constraint(constraint) / (delta_time * delta_time);
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
