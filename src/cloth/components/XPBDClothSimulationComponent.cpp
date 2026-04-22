#include "cloth/components/XPBDClothSimulationComponent.h"

XPBDClothSimulationComponent::XPBDClothSimulationComponent(Cloth& cloth)
	: ClothSimulationComponent(cloth)
{
	stretch_compliance_ = 0.000001f;
}

void XPBDClothSimulationComponent::prepare_distance_constraints(float)
{
	reset_constraint_lambdas();
}

float XPBDClothSimulationComponent::compute_distance_delta_lambda(
	DistanceConstraint& constraint,
	const DistanceConstraintSolveData& solve_data,
	float delta_time)
{
	const float alpha_tilde = constraint.compliance / (delta_time * delta_time);
	const float denominator = solve_data.inverse_mass_sum + alpha_tilde;

	if (denominator <= 0.0f)
	{
		return 0.0f;
	}

	const float delta_lambda = (-solve_data.constraint_value - alpha_tilde * constraint.lambda) / denominator;

	constraint.lambda += delta_lambda;
	return delta_lambda;
}
