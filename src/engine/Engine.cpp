#include "engine/Engine.h"

#include "engine/core/World.h"
#include "engine/render/RenderSystem.h"

#include <chrono>
#include <utility>

namespace
{
constexpr float kUpdateDeltaTime = 1.0f / 60.0f;
constexpr auto kUpdateInterval = std::chrono::milliseconds(16);
}

Engine::Engine(World& world, RenderSystem& render_system)
	: world_(world)
	, render_system_(render_system)
{
}

Engine::~Engine()
{
	shutdown();
}

void Engine::initialize()
{
	if (initialized_)
	{
		return;
	}

	world_.awake();
	world_.start();

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

	world_.start();
	engine_thread_ = std::thread(&Engine::run_engine_loop, this);
}

void Engine::stop()
{
	if (!running_.exchange(false))
	{
		return;
	}

	if (engine_thread_.joinable())
	{
		engine_thread_.join();
	}

	world_.stop();
}

void Engine::shutdown()
{
	stop();

	if (!initialized_)
	{
		return;
	}

	world_.destroy();

	initialized_ = false;
}

void Engine::set_render_request_callback(RenderRequestCallback render_request_callback)
{
	render_request_callback_ = std::move(render_request_callback);
}

void Engine::enqueue_world_job(WorldJob job)
{
	std::lock_guard<std::mutex> lock(world_job_mutex_);
	pending_world_jobs_.push_back(std::move(job));
}

void Engine::run_engine_loop()
{
	auto next_tick = std::chrono::steady_clock::now();

	while (running_)
	{
		next_tick += kUpdateInterval;

		tick(kUpdateDeltaTime);

		std::this_thread::sleep_until(next_tick);
	}
}

void Engine::tick(float delta_time)
{
	drain_world_jobs();
	world_.update(delta_time);
	publish_render_scene();

	if (render_request_callback_)
	{
		render_request_callback_();
	}
}

void Engine::drain_world_jobs()
{
	std::vector<WorldJob> jobs;

	{
		std::lock_guard<std::mutex> lock(world_job_mutex_);
		jobs.swap(pending_world_jobs_);
	}

	for (WorldJob& job : jobs)
	{
		if (job)
		{
			job(world_);
		}
	}
}

void Engine::publish_render_scene()
{
	render_system_.set_scene(world_.build_render_scene());
}
