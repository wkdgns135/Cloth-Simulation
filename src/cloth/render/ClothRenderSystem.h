#pragma once

#include <memory>

#include "cloth/ClothWorld.h"
#include "cloth/render/ClothRenderData.h"
#include "engine/render/RenderSystem.h"

class ClothRenderer;

class ClothRenderSystem final : public RenderSystem
{
public:
	explicit ClothRenderSystem(ClothWorld& world);
	~ClothRenderSystem();

	ClothRenderSystem(const ClothRenderSystem&) = delete;
	ClothRenderSystem& operator=(const ClothRenderSystem&) = delete;

	void initialize_gl();
	void shutdown_gl();
	void resize(int width, int height);
	void render();

private:
	ClothWorld& world_;
	std::unique_ptr<ClothRenderer> renderer_;
};
