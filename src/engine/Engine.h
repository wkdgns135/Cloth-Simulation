#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class RenderSystem;
class World;

class Engine final
{
public:
	using WorldJob = std::function<void(World&)>;
	using RenderRequestCallback = std::function<void()>;

	Engine(World& world, RenderSystem& render_system);
	~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void initialize();
	void start();
	void stop();
	void shutdown();

	void set_render_request_callback(RenderRequestCallback render_request_callback);
	void enqueue_world_job(WorldJob job);

private:
	void run_engine_loop();
	void tick(float delta_time);
	void drain_world_jobs();
	void publish_render_scene();

	World& world_;
	RenderSystem& render_system_;
	RenderRequestCallback render_request_callback_;
	std::mutex world_job_mutex_;
	std::vector<WorldJob> pending_world_jobs_;

	std::atomic<bool> running_ = false;
	bool initialized_ = false;

	std::thread engine_thread_;
};
