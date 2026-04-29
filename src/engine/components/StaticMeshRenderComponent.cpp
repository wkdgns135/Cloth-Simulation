#include "engine/components/StaticMeshRenderComponent.h"

#include <utility>

#include "engine/core/Object.h"

StaticMeshRenderComponent::StaticMeshRenderComponent(MeshData mesh, const glm::vec4& base_color)
	: mesh_(std::move(mesh))
	, base_color_(base_color)
{
}

void StaticMeshRenderComponent::collect_render_data(RenderScene& scene) const
{
	if (mesh_.vertices.empty() || mesh_.indices.empty())
	{
		return;
	}

	RenderObject object;
	object.mesh = mesh_;
	object.material.base_color = base_color_;

	if (const Object* object_owner = owner())
	{
		object.transform = object_owner->transform().matrix();
	}

	scene.objects.push_back(std::move(object));
}
