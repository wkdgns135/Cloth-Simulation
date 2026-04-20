#include "cloth/render/ClothRenderSystem.h"

#include "cloth/render/ClothRenderer.h"

ClothRenderSystem::ClothRenderSystem(ClothWorld& world)
	: world_(world)
{
}

ClothRenderSystem::~ClothRenderSystem() = default;

void ClothRenderSystem::initialize_gl()
{
	if (!renderer_)
	{
		renderer_ = std::make_unique<ClothRenderer>();
		renderer_->initialize();
	}
}

void ClothRenderSystem::shutdown_gl()
{
	renderer_.reset();
}

void ClothRenderSystem::resize(int width, int height)
{
	if (renderer_)
	{
		renderer_->resize(width, height);
	}
}

void ClothRenderSystem::render()
{
	if (!renderer_)
	{
		return;
	}

	ClothRenderData render_data;
	if (world_.consume_render_data(render_data))
	{
		renderer_->set_particles(render_data.positions);
	}

	renderer_->render();
}
