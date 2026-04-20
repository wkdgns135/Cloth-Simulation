#pragma once

#include "cloth/ClothWorld.h"
#include "cloth/physics/ClothSimulator.h"
#include "engine/physics/PhysicsSystem.h"

class ClothPhysicsSystem final : public PhysicsSystem
{
public:
	explicit ClothPhysicsSystem(ClothWorld& world);

	void update(float delta_time) override;

private:
	ClothWorld& world_;
	ClothSimulator simulator_;
};
