#pragma once

#include "engine/components/RenderComponent.h"
#include "engine/render/RenderScene.h"

class StaticMeshRenderComponent final : public RenderComponent
{
public:
	StaticMeshRenderComponent(MeshData mesh, const glm::vec4& base_color = glm::vec4(1.0f));
	PROPERTY(glm::vec3, base_color, "Render", "Base Color", glm::vec3(1.0f))

	void collect_render_data(RenderScene& scene) const override;

private:
	MeshData mesh_;
};
