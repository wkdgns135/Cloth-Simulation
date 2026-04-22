#pragma once

#include "cloth/components/ClothSimulationComponent.h"

class XPBDClothSimulationComponent final : public ClothSimulationComponent
{
public:
	explicit XPBDClothSimulationComponent(Cloth& cloth);

private:
	void prepare_distance_constraints(float delta_time) override;
	float compute_distance_delta_lambda(
		DistanceConstraint& constraint,
		const DistanceConstraintSolveData& solve_data,
		float delta_time) override;
};
