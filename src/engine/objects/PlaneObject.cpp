#include "engine/objects/PlaneObject.h"

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "engine/components/StaticMeshRenderComponent.h"

namespace
{
MeshData build_plane_mesh()
{
	MeshData mesh;
	mesh.vertices = {
		{ glm::vec3(-0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec4(1.0f) },
		{ glm::vec3(0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec4(1.0f) },
		{ glm::vec3(-0.5f, 0.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec4(1.0f) },
		{ glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec4(1.0f) },
	};
	mesh.indices = {
		0, 2, 1,
		1, 2, 3,
	};
	return mesh;
}
}

PlaneObject::PlaneObject()
{
	set_display_name("Plane");
	set_object_scale(glm::vec3(8.0f, 1.0f, 8.0f));
	add_component<StaticMeshRenderComponent>(build_plane_mesh(), glm::vec4(0.32f, 0.34f, 0.38f, 1.0f));
}

bool PlaneObject::hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const
{
	const glm::mat4 inverse_transform = glm::inverse(get_object_transform_matrix());
	const glm::vec3 local_ray_origin = glm::vec3(inverse_transform * glm::vec4(ray_origin, 1.0f));
	const glm::vec3 local_ray_direction =
		glm::normalize(glm::vec3(inverse_transform * glm::vec4(ray_direction, 0.0f)));

	if (std::abs(local_ray_direction.y) <= 0.000001f)
	{
		return false;
	}

	const float local_hit_distance = -local_ray_origin.y / local_ray_direction.y;
	if (local_hit_distance <= 0.000001f)
	{
		return false;
	}

	const glm::vec3 local_hit_point = local_ray_origin + local_ray_direction * local_hit_distance;
	if (std::abs(local_hit_point.x) > 0.5f || std::abs(local_hit_point.z) > 0.5f)
	{
		return false;
	}

	const float world_hit_distance = glm::length(glm::vec3(get_object_transform_matrix() * glm::vec4(local_hit_point, 1.0f)) - ray_origin);
	if (world_hit_distance >= hit_distance)
	{
		return false;
	}

	hit_distance = world_hit_distance;
	return true;
}

bool PlaneObject::resolve_particle_collision(
	glm::vec3& world_position,
	glm::vec3& world_prev_position,
	float margin) const
{
	const glm::vec3 plane_normal = get_object_up_direction();
	const glm::vec3 plane_point = get_object_world_position();
	const float signed_distance = glm::dot(plane_normal, world_position - plane_point);

	if (signed_distance >= margin)
	{
		return false;
	}

	world_position += (margin - signed_distance) * plane_normal;

	const glm::vec3 world_velocity = world_position - world_prev_position;
	const float normal_velocity = glm::dot(world_velocity, plane_normal);
	if (normal_velocity < 0.0f)
	{
		world_prev_position += normal_velocity * plane_normal;
	}

	return true;
}
