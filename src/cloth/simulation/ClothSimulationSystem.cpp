#include "cloth/simulation/ClothSimulationSystem.h"

ClothSimulationSystem::ClothSimulationSystem(ClothWorld& world)
	: world_(world)
{
}

void ClothSimulationSystem::awake()
{
	world_.initialize();
	world_.publish_render_data();
}

void ClothSimulationSystem::update(float)
{
	world_.publish_render_data();
}
