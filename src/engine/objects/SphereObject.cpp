#include "engine/objects/SphereObject.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>

#include "engine/components/StaticMeshRenderComponent.h"

namespace
{
constexpr float kCollisionEpsilon = 0.000001f;
constexpr int kSphereSlices = 24;
constexpr int kSphereStacks = 12;

MeshData build_sphere_mesh(float radius)
{
	MeshData mesh;
	mesh.vertices.reserve(static_cast<std::size_t>((kSphereStacks + 1) * (kSphereSlices + 1)));
	mesh.indices.reserve(static_cast<std::size_t>(kSphereStacks * kSphereSlices * 6));

	for (int stack = 0; stack <= kSphereStacks; ++stack)
	{
		const float v = static_cast<float>(stack) / static_cast<float>(kSphereStacks);
		const float phi = v * glm::pi<float>();
		const float y = std::cos(phi);
		const float ring_radius = std::sin(phi);

		for (int slice = 0; slice <= kSphereSlices; ++slice)
		{
			const float u = static_cast<float>(slice) / static_cast<float>(kSphereSlices);
			const float theta = u * glm::two_pi<float>();
			const glm::vec3 normal(
				std::cos(theta) * ring_radius,
				y,
				std::sin(theta) * ring_radius);

			RenderVertex vertex;
			vertex.position = radius * normal;
			vertex.normal = glm::dot(normal, normal) > 0.0f ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);
			vertex.color = glm::vec4(1.0f);
			mesh.vertices.push_back(vertex);
		}
	}

	for (int stack = 0; stack < kSphereStacks; ++stack)
	{
		for (int slice = 0; slice < kSphereSlices; ++slice)
		{
			const unsigned int current = static_cast<unsigned int>(stack * (kSphereSlices + 1) + slice);
			const unsigned int next = current + static_cast<unsigned int>(kSphereSlices + 1);

			mesh.indices.push_back(current);
			mesh.indices.push_back(next);
			mesh.indices.push_back(current + 1);

			mesh.indices.push_back(current + 1);
			mesh.indices.push_back(next);
			mesh.indices.push_back(next + 1);
		}
	}

	return mesh;
}
}

SphereObject::SphereObject(float radius)
	: radius_(std::max(radius, 0.0f))
{
	add_component<StaticMeshRenderComponent>(build_sphere_mesh(radius_), glm::vec4(0.72f, 0.28f, 0.24f, 1.0f));
}

void SphereObject::configure_projectile(const glm::vec3& velocity, float max_travel_distance)
{
	linear_velocity_ = velocity;
	spawn_position_ = transform().position;
	max_travel_distance_ = std::max(max_travel_distance, 0.0f);
	lifetime_limit_enabled_ = max_travel_distance_ > 0.0f;
}

void SphereObject::update(float delta_time)
{
	WorldObject::update(delta_time);

	if (delta_time <= 0.0f || destroy_requested())
	{
		return;
	}

	transform().position += linear_velocity_ * delta_time;

	if (!lifetime_limit_enabled_)
	{
		return;
	}

	const glm::vec3 delta = transform().position - spawn_position_;
	if (glm::dot(delta, delta) >= max_travel_distance_ * max_travel_distance_)
	{
		request_destroy();
	}
}

bool SphereObject::resolve_particle_collision(
	glm::vec3& world_position,
	glm::vec3& world_prev_position,
	float margin) const
{
	const glm::vec3 center = transform().position;
	const float target_radius = world_radius() + margin;
	const glm::vec3 delta = world_position - center;
	const float distance_squared = glm::dot(delta, delta);

	if (distance_squared >= target_radius * target_radius)
	{
		return false;
	}

	glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
	if (distance_squared > kCollisionEpsilon)
	{
		normal = delta / std::sqrt(distance_squared);
	}
	else
	{
		const glm::vec3 motion = world_position - world_prev_position;
		if (glm::dot(motion, motion) > kCollisionEpsilon)
		{
			normal = glm::normalize(motion);
		}
	}

	world_position = center + target_radius * normal;

	const glm::vec3 world_velocity = world_position - world_prev_position;
	const float normal_velocity = glm::dot(world_velocity, normal);
	if (normal_velocity < 0.0f)
	{
		world_prev_position += normal_velocity * normal;
	}

	return true;
}

float SphereObject::world_radius() const
{
	const glm::vec3 scale = transform().scale;
	const glm::vec3 abs_scale(std::abs(scale.x), std::abs(scale.y), std::abs(scale.z));
	return radius_ * std::max({ abs_scale.x, abs_scale.y, abs_scale.z });
}
