#pragma once

#include "engine/components/RenderComponent.h"

class Cloth;

class ClothRenderComponent final : public RenderComponent
{
public:
	explicit ClothRenderComponent(const Cloth& cloth);

	void collect_render_data(RenderScene& scene) const override;

private:
	const Cloth& cloth_;
};
