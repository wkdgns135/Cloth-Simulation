#include "engine/render/RenderSystem.h"

#include "engine/render/Renderer.h"
#include "engine/render/RenderScene.h"

#include <utility>

RenderSystem::RenderSystem()
{
}

RenderSystem::~RenderSystem() = default;

void RenderSystem::initialize_gl()
{
	if (!renderer_)
	{
		renderer_ = std::make_unique<Renderer>();
		renderer_->initialize();
	}
}

void RenderSystem::shutdown_gl()
{
	renderer_.reset();

	std::lock_guard<std::mutex> lock(scene_mutex_);
	latest_scene_.reset();
}

void RenderSystem::resize(int width, int height)
{
	if (renderer_)
	{
		renderer_->resize(width, height);
	}
}

void RenderSystem::render()
{
	if (!renderer_)
	{
		return;
	}

	{
		std::shared_ptr<const RenderScene> scene;
		{
			std::lock_guard<std::mutex> lock(scene_mutex_);
			scene = latest_scene_;
		}

		if (scene)
		{
			renderer_->render(*scene);
			return;
		}
	}

	renderer_->render(RenderScene{});
}

void RenderSystem::set_scene(RenderScene scene)
{
	auto next_scene = std::make_shared<RenderScene>(std::move(scene));

	std::lock_guard<std::mutex> lock(scene_mutex_);
	latest_scene_ = std::move(next_scene);
}
