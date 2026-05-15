#include "cloth/components/ClothRenderComponent.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>
#include <glm/geometric.hpp>

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"
#include "engine/core/WorldObject.h"
#include "engine/render/RenderScene.h"

namespace
{
constexpr float kDegenerateNormalThreshold = 0.000000000001f;
constexpr float kDisplacementColorThreshold = 0.0000001f;

std::uint64_t make_edge_key(unsigned int vertex_a, unsigned int vertex_b)
{
	const unsigned int min_vertex = std::min(vertex_a, vertex_b);
	const unsigned int max_vertex = std::max(vertex_a, vertex_b);
	return (static_cast<std::uint64_t>(min_vertex) << 32) | static_cast<std::uint64_t>(max_vertex);
}

glm::vec3 lerp_color(const glm::vec3& from, const glm::vec3& to, float t)
{
	return from + (to - from) * t;
}

glm::vec4 displacement_heat_color(float displacement, float max_visible_displacement)
{
	const float t =
		max_visible_displacement > kDisplacementColorThreshold
		? std::clamp(displacement / max_visible_displacement, 0.0f, 1.0f)
		: 0.0f;
	const glm::vec3 low(0.08f, 0.22f, 0.95f);
	const glm::vec3 mid(0.95f, 0.86f, 0.12f);
	const glm::vec3 high(1.0f, 0.12f, 0.04f);

	if (t < 0.5f)
	{
		return glm::vec4(lerp_color(low, mid, t * 2.0f), 1.0f);
	}

	return glm::vec4(lerp_color(mid, high, (t - 0.5f) * 2.0f), 1.0f);
}

std::vector<unsigned int> build_skeleton_indices(
	const std::vector<unsigned int>& triangle_indices,
	std::size_t vertex_count)
{
	std::vector<unsigned int> line_indices;
	std::unordered_set<std::uint64_t> added_edges;
	added_edges.reserve(triangle_indices.size());
	line_indices.reserve(triangle_indices.size() * 2);

	const auto add_edge = [&](unsigned int vertex_a, unsigned int vertex_b)
	{
		if (vertex_a >= vertex_count || vertex_b >= vertex_count || vertex_a == vertex_b)
		{
			return;
		}

		const std::uint64_t edge_key = make_edge_key(vertex_a, vertex_b);
		if (!added_edges.insert(edge_key).second)
		{
			return;
		}

		line_indices.push_back(vertex_a);
		line_indices.push_back(vertex_b);
	};

	for (std::size_t i = 0; i + 2 < triangle_indices.size(); i += 3)
	{
		const unsigned int i0 = triangle_indices[i];
		const unsigned int i1 = triangle_indices[i + 1];
		const unsigned int i2 = triangle_indices[i + 2];
		add_edge(i0, i1);
		add_edge(i1, i2);
		add_edge(i2, i0);
	}

	return line_indices;
}

std::vector<glm::vec3> build_vertex_normals(
	const std::vector<Particle>& particles,
	const std::vector<unsigned int>& indices)
{
	std::vector<glm::vec3> normals(particles.size(), glm::vec3(0.0f));

	for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const unsigned int i0 = indices[i];
		const unsigned int i1 = indices[i + 1];
		const unsigned int i2 = indices[i + 2];

		if (i0 >= particles.size() || i1 >= particles.size() || i2 >= particles.size())
		{
			continue;
		}

		const glm::vec3 edge_01 = particles[i1].position - particles[i0].position;
		const glm::vec3 edge_02 = particles[i2].position - particles[i0].position;
		const glm::vec3 face_normal = glm::cross(edge_01, edge_02);

		if (glm::dot(face_normal, face_normal) <= kDegenerateNormalThreshold)
		{
			continue;
		}

		normals[i0] += face_normal;
		normals[i1] += face_normal;
		normals[i2] += face_normal;
	}

	for (glm::vec3& normal : normals)
	{
		if (glm::dot(normal, normal) <= kDegenerateNormalThreshold)
		{
			normal = glm::vec3(0.0f, 0.0f, 1.0f);
			continue;
		}

		normal = glm::normalize(normal);
	}

	return normals;
}
}

ClothRenderComponent::ClothRenderComponent(const Cloth& cloth)
	: cloth_(cloth)
{
	set_display_name("Cloth Renderer");
}

void ClothRenderComponent::collect_render_data(RenderScene& scene) const
{
	const std::vector<Particle>& particles = cloth_.get_particles();
	const std::vector<unsigned int>& indices = cloth_.get_indices();

	if (particles.empty() || indices.empty())
	{
		return;
	}

	const std::vector<glm::vec3> normals = build_vertex_normals(particles, indices);
	RenderObject object;
	object.mesh.vertices.reserve(particles.size());
	if (skeleton_render_enabled())
	{
		object.mesh.indices = build_skeleton_indices(indices, particles.size());
		object.primitive_topology = RenderPrimitiveTopology::Lines;
	}
	else
	{
		object.mesh.indices = indices;
	}

	if (object.mesh.indices.empty())
	{
		return;
	}

	object.material.base_color = glm::vec4(1.0f);
	const glm::vec4 cloth_color(surface_color(), 1.0f);
	const bool use_displacement_colors = displacement_color_enabled();
	const float max_visible_displacement = displacement_color_max();

	if (const WorldObject* object_owner = owner())
	{
		object.transform = object_owner->get_object_transform_matrix();
	}

	for (std::size_t i = 0; i < particles.size(); ++i)
	{
		RenderVertex vertex;
		vertex.position = particles[i].position;
		vertex.normal = normals[i];
		if (use_displacement_colors)
		{
			const float displacement = glm::length(particles[i].position - particles[i].prev_position);
			vertex.color = displacement_heat_color(displacement, max_visible_displacement);
		}
		else
		{
			vertex.color = cloth_color;
		}
		object.mesh.vertices.push_back(vertex);
	}

	scene.objects.push_back(std::move(object));
}
