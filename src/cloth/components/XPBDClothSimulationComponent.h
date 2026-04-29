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

	void set_stretch_compliance(float compliance) { stretch_compliance_ = std::max(0.0f, compliance); }
	float stretch_compliance() const { return stretch_compliance_; }

	void set_bend_compliance(float compliance) { bend_compliance_ = std::max(0.0f, compliance); }
	float bend_compliance() const { return bend_compliance_; }

private:
	void sync_constraint_states();
	void reset_constraint_lambdas();
	void solve_distance_constraints(float delta_time);
	void solve_distance_constraint(std::size_t constraint_index, float delta_time);
	void solve_bending_constraints(float delta_time);
	void solve_bending_constraint(std::size_t constraint_index, float delta_time);

	std::vector<float> distance_constraint_lambdas_;
	std::vector<float> bending_constraint_lambdas_;
	float stretch_compliance_ = 0.000001f;
	float bend_compliance_ = 0.0005f;
};
