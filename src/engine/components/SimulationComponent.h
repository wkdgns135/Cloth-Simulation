#pragma once

#include "engine/core/Component.h"

class SimulationComponent : public Component
{
public:
	~SimulationComponent() override = default;

	void update(float delta_time) override
	{
		update_simulation(delta_time);
	}

	virtual void update_simulation(float delta_time) = 0;
};
