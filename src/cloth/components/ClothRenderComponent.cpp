#include "cloth/components/ClothRenderComponent.h"

#include <cstddef>
#include <utility>
#include <vector>
#include <glm/geometric.hpp>

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"
#include "engine/core/Object.h"
#include "engine/render/RenderScene.h"

namespace
{
constexpr float kDegenerateNormalThreshold = 0.000000000001f;

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
	const glm::vec4 cloth_color(0.62f, 0.62f, 0.60f, 1.0f);

	RenderObject object;
	object.mesh.vertices.reserve(particles.size());
	object.mesh.indices = indices;
	object.material.base_color = glm::vec4(1.0f);

	if (const Object* object_owner = owner())
	{
		object.transform = object_owner->transform().matrix();
	}

	for (std::size_t i = 0; i < particles.size(); ++i)
	{
		RenderVertex vertex;
		vertex.position = particles[i].position;
		vertex.normal = normals[i];
		vertex.color = cloth_color;
		object.mesh.vertices.push_back(vertex);
	}

	scene.objects.push_back(std::move(object));
}
