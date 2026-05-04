#pragma once

#include "engine/components/RenderComponent.h"

class Cloth;

class ClothRenderComponent final : public RenderComponent
{
public:
	explicit ClothRenderComponent(const Cloth& cloth);
	PROPERTY(glm::vec3, surface_color, "Render", "Surface Color", glm::vec3(0.62f, 0.62f, 0.60f))

	void collect_render_data(RenderScene& scene) const override;

private:
	const Cloth& cloth_;
};
