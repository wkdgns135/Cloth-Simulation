#pragma once

#include "cloth/components/ClothSimulationComponentBase.h"

class Cloth;

class PBDClothSimulationComponent final : public ClothSimulationComponentBase
{
public:
	explicit PBDClothSimulationComponent(Cloth& cloth);

	void update_simulation(float delta_time) override;

	void set_stretch_stiffness(float stiffness) { stretch_stiffness_ = std::clamp(stiffness, 0.0f, 1.0f); }
	float stretch_stiffness() const { return stretch_stiffness_; }

	void set_bend_stiffness(float stiffness) { bend_stiffness_ = std::clamp(stiffness, 0.0f, 1.0f); }
	float bend_stiffness() const { return bend_stiffness_; }

private:
	void solve_distance_constraints(float stiffness_per_iteration);
	void solve_distance_constraint(const DistanceConstraint& constraint, float stiffness_per_iteration);
	void solve_bending_constraints(float stiffness_per_iteration);
	void solve_bending_constraint(const BendingConstraint& constraint, float stiffness_per_iteration);

	float stretch_stiffness_ = 1.0f;
	float bend_stiffness_ = 0.15f;
};
