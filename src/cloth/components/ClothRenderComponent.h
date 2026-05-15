#pragma once

#include "engine/components/RenderComponent.h"

class Cloth;

class ClothRenderComponent final : public RenderComponent
{
public:
	explicit ClothRenderComponent(const Cloth& cloth);
	PROPERTY(glm::vec3, surface_color, "Render", "Surface Color", glm::vec3(0.62f, 0.62f, 0.60f))
	PROPERTY(bool, displacement_color_enabled, "Render", "Displacement Colors", false)
	PROPERTY_RANGE(float, displacement_color_max, "Render", "Displacement Color Max", 0.005f, 0.0001f, 1.0f, 0.001f)
	PROPERTY(bool, skeleton_render_enabled, "Render", "Skeleton", false)

	void collect_render_data(RenderScene& scene) const override;

private:
	const Cloth& cloth_;
};
