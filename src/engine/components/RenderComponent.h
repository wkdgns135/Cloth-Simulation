#pragma once

#include "engine/core/Component.h"

struct RenderScene;

class RenderComponent : public Component
{
public:
	~RenderComponent() override = default;

	virtual void collect_render_data(RenderScene& scene) const = 0;
};
