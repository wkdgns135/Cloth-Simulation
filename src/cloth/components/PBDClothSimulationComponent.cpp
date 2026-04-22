#include "cloth/components/PBDClothSimulationComponent.h"

PBDClothSimulationComponent::PBDClothSimulationComponent(Cloth& cloth)
	: ClothSimulationComponent(cloth)
{
}

float PBDClothSimulationComponent::compute_distance_delta_lambda(
	DistanceConstraint&,
	const DistanceConstraintSolveData& solve_data,
	float)
{
	return -solve_data.constraint_value / solve_data.inverse_mass_sum;
}
