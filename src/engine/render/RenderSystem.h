#pragma once

#include <memory>
#include <mutex>

struct RenderScene;
class Renderer;

class RenderSystem final
{
public:
	RenderSystem();
	~RenderSystem();

	RenderSystem(const RenderSystem&) = delete;
	RenderSystem& operator=(const RenderSystem&) = delete;

	void initialize_gl();
	void shutdown_gl();
	void resize(int width, int height);
	void render();
	void set_scene(RenderScene scene);

private:
	std::unique_ptr<Renderer> renderer_;
	std::shared_ptr<const RenderScene> latest_scene_;
	std::mutex scene_mutex_;
};
