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
	PROPERTY_RANGE(float, stretch_compliance, "Solver", "Stretch Compliance", 0.0f, 0.0f, 0.1f, 0.000001f)
	PROPERTY_RANGE(float, shear_compliance, "Solver", "Shear Compliance", 0.0001f, 0.0f, 0.1f, 0.000001f)
	PROPERTY_RANGE(float, bend_compliance, "Solver", "Bend Compliance", 1.0f, 0.0f, 10.0f, 0.0001f)

private:
	float compliance_for_constraint(const DistanceConstraint& constraint) const;
	void sync_constraint_states();
	void reset_constraint_lambdas();
	void solve_constraints(float delta_time);
	void solve_distance_constraint(std::size_t constraint_index, float delta_time);

	std::vector<float> distance_constraint_lambdas_;
};
