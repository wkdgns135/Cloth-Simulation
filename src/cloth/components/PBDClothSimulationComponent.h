#pragma once

#include "cloth/components/ClothSimulationComponentBase.h"

class Cloth;

class PBDClothSimulationComponent final : public ClothSimulationComponentBase
{
public:
	explicit PBDClothSimulationComponent(Cloth& cloth);

	void update_simulation(float delta_time) override;
	PROPERTY_RANGE(float, stretch_stiffness, "Solver", "Stretch Stiffness", 1.0f, 0.0f, 1.0f, 0.01f)
	PROPERTY_RANGE(float, bend_stiffness, "Solver", "Bend Stiffness", 0.15f, 0.0f, 1.0f, 0.01f)

private:
	void solve_distance_constraints(float stiffness_per_iteration);
	void solve_distance_constraint(const DistanceConstraint& constraint, float stiffness_per_iteration);
	void solve_bending_constraints(float stiffness_per_iteration);
	void solve_bending_constraint(const BendingConstraint& constraint, float stiffness_per_iteration);
};
