#include "engine/Engine.h"

#include <chrono>

namespace
{
constexpr float kSimulationDeltaTime = 1.0f / 60.0f;
constexpr float kPhysicsDeltaTime = 1.0f / 60.0f;
constexpr float kRenderDeltaTime = 1.0f / 60.0f;

constexpr auto kSimulationInterval = std::chrono::milliseconds(16);
constexpr auto kPhysicsInterval = std::chrono::milliseconds(16);
constexpr auto kRenderInterval = std::chrono::milliseconds(16);
}

Engine::Engine() = default;

Engine::~Engine()
{
	shutdown();
}

void Engine::add_simulation_system(SimulationSystem& system)
{
	lifecycle_systems_.push_back(&system);
	simulation_systems_.push_back(&system);
}

void Engine::add_physics_system(PhysicsSystem& system)
{
	lifecycle_systems_.push_back(&system);
	physics_systems_.push_back(&system);
}

void Engine::add_render_system(RenderSystem& system)
{
	lifecycle_systems_.push_back(&system);
	render_systems_.push_back(&system);
}

void Engine::initialize()
{
	if (initialized_)
	{
		return;
	}

	awake_all();
	start_all();

	initialized_ = true;
}

void Engine::start()
{
	if (!initialized_)
	{
		initialize();
	}

	if (running_.exchange(true))
	{
		return;
	}

	simulation_thread_ = std::thread(&Engine::run_simulation_loop, this);
	physics_thread_ = std::thread(&Engine::run_physics_loop, this);
	render_thread_ = std::thread(&Engine::run_render_loop, this);
}

void Engine::stop()
{
	if (!running_.exchange(false))
	{
		return;
	}

	if (simulation_thread_.joinable())
	{
		simulation_thread_.join();
	}

	if (physics_thread_.joinable())
	{
		physics_thread_.join();
	}

	if (render_thread_.joinable())
	{
		render_thread_.join();
	}

	stop_all();
}

void Engine::shutdown()
{
	stop();

	if (!initialized_)
	{
		return;
	}

	destroy_all();

	initialized_ = false;
}

void Engine::run_simulation_loop()
{
	run_loop(simulation_systems_, kSimulationDeltaTime, kSimulationInterval);
}

void Engine::run_physics_loop()
{
	run_loop(physics_systems_, kPhysicsDeltaTime, kPhysicsInterval);
}

void Engine::run_render_loop()
{
	run_loop(render_systems_, kRenderDeltaTime, kRenderInterval);
}

void Engine::run_loop(
	const std::vector<Lifecycle*>& systems,
	float delta_time,
	std::chrono::milliseconds interval)
{
	auto next_tick = std::chrono::steady_clock::now();

	while (running_)
	{
		next_tick += interval;

		for (Lifecycle* system : systems)
		{
			system->update(delta_time);
		}

		std::this_thread::sleep_until(next_tick);
	}
}

void Engine::awake_all()
{
	for (Lifecycle* system : lifecycle_systems_)
	{
		system->awake();
	}
}

void Engine::start_all()
{
	for (Lifecycle* system : lifecycle_systems_)
	{
		system->start();
	}
}

void Engine::stop_all()
{
	for (Lifecycle* system : lifecycle_systems_)
	{
		system->stop();
	}
}

void Engine::destroy_all()
{
	for (Lifecycle* system : lifecycle_systems_)
	{
		system->destroy();
	}
}
