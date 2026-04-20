#pragma once

#include "engine/render/RenderSystem.h"

class MainWindow;

class QtRenderRequestSystem final : public RenderSystem
{
public:
	explicit QtRenderRequestSystem(MainWindow& window);

	void update(float delta_time) override;

private:
	MainWindow& window_;
};
