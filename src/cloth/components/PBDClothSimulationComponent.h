#pragma once

#include "cloth/components/ClothSimulationComponent.h"

class PBDClothSimulationComponent final : public ClothSimulationComponent
{
public:
	explicit PBDClothSimulationComponent(Cloth& cloth);

private:
	float compute_distance_delta_lambda(
		DistanceConstraint& constraint,
		const DistanceConstraintSolveData& solve_data,
		float delta_time) override;
};
