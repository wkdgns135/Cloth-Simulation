#include "engine/components/StaticMeshRenderComponent.h"

#include <utility>

#include "engine/core/WorldObject.h"

StaticMeshRenderComponent::StaticMeshRenderComponent(MeshData mesh, const glm::vec4& base_color)
	: mesh_(std::move(mesh))
{
	set_display_name("Static Mesh Renderer");
	set_base_color(glm::vec3(base_color));
}

void StaticMeshRenderComponent::collect_render_data(RenderScene& scene) const
{
	if (mesh_.vertices.empty() || mesh_.indices.empty())
	{
		return;
	}

	RenderObject object;
	object.mesh = mesh_;
	object.material.base_color = glm::vec4(base_color(), 1.0f);

	if (const WorldObject* object_owner = owner())
	{
		object.transform = object_owner->get_object_transform_matrix();
	}

	scene.objects.push_back(std::move(object));
}
