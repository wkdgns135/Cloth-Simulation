#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "engine/lifecycle/Lifecycle.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/render/RenderSystem.h"
#include "engine/simulation/SimulationSystem.h"

class Engine final
{
public:
	Engine();
	~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void add_simulation_system(SimulationSystem& system);
	void add_physics_system(PhysicsSystem& system);
	void add_render_system(RenderSystem& system);

	void initialize();
	void start();
	void stop();
	void shutdown();

private:
	void run_simulation_loop();
	void run_physics_loop();
	void run_render_loop();

	void run_loop(
		const std::vector<Lifecycle*>& systems,
		float delta_time,
		std::chrono::milliseconds interval);

	void awake_all();
	void start_all();
	void stop_all();
	void destroy_all();

	std::vector<Lifecycle*> lifecycle_systems_;
	std::vector<Lifecycle*> simulation_systems_;
	std::vector<Lifecycle*> physics_systems_;
	std::vector<Lifecycle*> render_systems_;

	std::atomic<bool> running_ = false;
	bool initialized_ = false;

	std::thread simulation_thread_;
	std::thread physics_thread_;
	std::thread render_thread_;
};
