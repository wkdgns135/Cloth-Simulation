#pragma once

#include "cloth/ClothWorld.h"
#include "engine/simulation/SimulationSystem.h"

class ClothSimulationSystem final : public SimulationSystem
{
public:
	explicit ClothSimulationSystem(ClothWorld& world);

	void awake() override;
	void update(float delta_time) override;

private:
	ClothWorld& world_;
};
