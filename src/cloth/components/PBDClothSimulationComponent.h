#pragma once

#include "cloth/components/ClothSimulationComponentBase.h"

class Cloth;

class PBDClothSimulationComponent final : public ClothSimulationComponentBase
{
public:
	explicit PBDClothSimulationComponent(Cloth& cloth);

	void update_simulation(float delta_time) override;
	PROPERTY_RANGE(float, stretch_stiffness, "Solver", "Stretch Stiffness", 1.0f, 0.0f, 1.0f, 0.01f)
	PROPERTY_RANGE(float, shear_stiffness, "Solver", "Shear Stiffness", 0.2f, 0.0f, 1.0f, 0.01f)
	PROPERTY_RANGE(float, bend_stiffness, "Solver", "Bend Stiffness", 0.05f, 0.0f, 1.0f, 0.01f)

private:
	float stiffness_for_constraint(const DistanceConstraint& constraint) const;
	void solve_constraints();
	void solve_distance_constraint(const DistanceConstraint& constraint);
};
