#include "cloth/physics/ClothPhysicsSystem.h"

ClothPhysicsSystem::ClothPhysicsSystem(ClothWorld& world)
	: world_(world)
{
}

void ClothPhysicsSystem::update(float delta_time)
{
	world_.step_physics(simulator_, delta_time);
}
