#pragma once

#include <vector>

#include "cloth/components/ClothSimulationComponentBase.h"

class Cloth;

class XPBDClothSimulationComponent final : public ClothSimulationComponentBase
{
public:
	explicit XPBDClothSimulationComponent(Cloth& cloth);

	void start() override;
	void update_simulation(float delta_time) override;
	PROPERTY_RANGE(float, stretch_compliance, "Solver", "Stretch Compliance", 0.000001f, 0.0f, 0.1f, 0.000001f)
	PROPERTY_RANGE(float, bend_compliance, "Solver", "Bend Compliance", 0.0005f, 0.0f, 0.1f, 0.000001f)

private:
	void sync_constraint_states();
	void reset_constraint_lambdas();
	void solve_distance_constraints(float delta_time);
	void solve_distance_constraint(std::size_t constraint_index, float delta_time);
	void solve_bending_constraints(float delta_time);
	void solve_bending_constraint(std::size_t constraint_index, float delta_time);

	std::vector<float> distance_constraint_lambdas_;
	std::vector<float> bending_constraint_lambdas_;
};
