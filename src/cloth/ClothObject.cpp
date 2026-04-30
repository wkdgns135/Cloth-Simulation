#include "cloth/ClothObject.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <vector>
#include <cfloat>

#include <glm/ext/scalar_relational.hpp>

#include "cloth/ClothWorld.h"
#include "components/ClothInteractionComponent.h"
#include "components/ClothRenderComponent.h"
#include "components/ClothSimulationComponentBase.h"
#include "components/PBDClothSimulationComponent.h"
#include "components/XPBDClothSimulationComponent.h"
#include "io/MeshLoader.h"

namespace
{
constexpr float kRayIntersectionEpsilon = 0.000001f;

std::string make_grid_source_label(int width, int height, float spacing)
{
	std::ostringstream stream;
	stream << width << " x " << height << " @ " << spacing;
	return stream.str();
}

void pin_grid_top_row(Cloth& cloth)
{
	for (int i = 0; i < cloth.get_width(); ++i)
	{
		cloth.set_fixed(i, 0, true);
	}
}

void pin_highest_particles(Cloth& cloth)
{
	auto& particles = cloth.get_particles();
	if (particles.empty())
	{
		return;
	}

	float max_height = -FLT_MAX;
	for (const Particle& particle : particles)
	{
		max_height = std::max(max_height, particle.position.y);
	}
	
	for (Particle& particle : particles)
	{
		if (glm::equal(particle.position.y, max_height, 1e-5f))
		{
			particle.is_fixed = true;
		}
	}
}

bool intersect_triangle(
	const glm::vec3& ray_origin,
	const glm::vec3& ray_direction,
	const glm::vec3& p0,
	const glm::vec3& p1,
	const glm::vec3& p2,
	float& hit_distance)
{
	const glm::vec3 edge_01 = p1 - p0;
	const glm::vec3 edge_02 = p2 - p0;
	const glm::vec3 pvec = glm::cross(ray_direction, edge_02);
	const float determinant = glm::dot(edge_01, pvec);

	if (std::abs(determinant) <= kRayIntersectionEpsilon)
	{
		return false;
	}

	const float inverse_determinant = 1.0f / determinant;
	const glm::vec3 tvec = ray_origin - p0;
	const float u = glm::dot(tvec, pvec) * inverse_determinant;
	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	const glm::vec3 qvec = glm::cross(tvec, edge_01);
	const float v = glm::dot(ray_direction, qvec) * inverse_determinant;
	if (v < 0.0f || (u + v) > 1.0f)
	{
		return false;
	}

	const float t = glm::dot(edge_02, qvec) * inverse_determinant;
	if (t <= kRayIntersectionEpsilon)
	{
		return false;
	}

	hit_distance = t;
	return true;
}
}

ClothObject::ClothObject(std::string display_name, int width, int height, float spacing)
	: WorldObject(std::move(display_name))
	, source_label_(make_grid_source_label(width, height, spacing))
	, source_kind_(ClothSourceKind::Grid)
	, cloth_(width, height, spacing)
{
	pin_grid_top_row(cloth_);
	cache_initial_state();
	add_component<ClothRenderComponent>(cloth_);
	add_component<ClothInteractionComponent>(*this);
}

ClothObject::ClothObject(std::string display_name, const std::filesystem::path& mesh_path)
	: WorldObject(std::move(display_name))
	, source_label_(mesh_path.filename().string())
	, source_kind_(ClothSourceKind::Mesh)
	, cloth_(io::load_cloth(mesh_path))
{
	pin_highest_particles(cloth_);
	cache_initial_state();
	add_component<ClothRenderComponent>(cloth_);
	add_component<ClothInteractionComponent>(*this);
}

ClothSolverKind ClothObject::solver_kind() const
{
	if (find_component<XPBDClothSimulationComponent>())
	{
		return ClothSolverKind::XPBD;
	}

	return ClothSolverKind::PBD;
}

ClothSimulationComponentBase* ClothObject::simulation_component()
{
	return find_component<ClothSimulationComponentBase>();
}

const ClothSimulationComponentBase* ClothObject::simulation_component() const
{
	return find_component<ClothSimulationComponentBase>();
}

void ClothObject::reset_to_initial_state()
{
	refresh_initial_state_if_needed();
	cloth_.get_particles() = initial_particles_;
	anchors_enabled_ = true;
}

void ClothObject::toggle_anchor_state()
{
	refresh_initial_state_if_needed();
	anchors_enabled_ = !anchors_enabled_;

	auto& particles = cloth_.get_particles();
	const std::size_t particle_count = std::min(particles.size(), initial_particles_.size());

	for (std::size_t i = 0; i < particle_count; ++i)
	{
		particles[i].is_fixed = anchors_enabled_ ? initial_particles_[i].is_fixed : false;

		if (particles[i].is_fixed)
		{
			particles[i].prev_position = particles[i].position;
		}
	}
}

bool ClothObject::hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const
{
	const std::vector<Particle>& particles = cloth_.get_particles();
	const std::vector<unsigned int>& indices = cloth_.get_indices();
	if (particles.empty() || indices.size() < 3)
	{
		return false;
	}

	const glm::mat4 object_transform = transform().matrix();
	bool has_hit = false;
	float closest_distance = hit_distance;

	for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const unsigned int i0 = indices[i];
		const unsigned int i1 = indices[i + 1];
		const unsigned int i2 = indices[i + 2];

		if (i0 >= particles.size() || i1 >= particles.size() || i2 >= particles.size())
		{
			continue;
		}

		const glm::vec3 p0 = glm::vec3(object_transform * glm::vec4(particles[i0].position, 1.0f));
		const glm::vec3 p1 = glm::vec3(object_transform * glm::vec4(particles[i1].position, 1.0f));
		const glm::vec3 p2 = glm::vec3(object_transform * glm::vec4(particles[i2].position, 1.0f));

		float triangle_hit_distance = 0.0f;
		if (!intersect_triangle(ray_origin, ray_direction, p0, p1, p2, triangle_hit_distance))
		{
			continue;
		}

		if (!has_hit || triangle_hit_distance < closest_distance)
		{
			has_hit = true;
			closest_distance = triangle_hit_distance;
		}
	}

	if (has_hit)
	{
		hit_distance = closest_distance;
	}

	return has_hit;
}

bool ClothObject::on_click(const ClickInputEvent& event)
{
	static_cast<void>(event);

	if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(world()))
	{
		cloth_world->select_cloth(id());
		return true;
	}

	return false;
}

void ClothObject::cache_initial_state()
{
	initial_particles_ = cloth_.get_particles();
	cached_topology_revision_ = cloth_.get_topology_revision();
	anchors_enabled_ = true;
}

void ClothObject::refresh_initial_state_if_needed()
{
	if (cached_topology_revision_ != cloth_.get_topology_revision()
		|| initial_particles_.size() != cloth_.get_particles().size())
	{
		cache_initial_state();
	}
}
