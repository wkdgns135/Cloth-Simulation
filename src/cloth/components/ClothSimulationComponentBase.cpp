#include "cloth/components/ClothSimulationComponentBase.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"
#include "engine/core/WorldObject.h"
#include "engine/core/World.h"
#include "engine/objects/CollisionObject.h"

namespace
{
constexpr float kConstraintLengthThreshold = 0.000001f;

std::uint64_t make_edge_key(int particle_a, int particle_b)
{
	const std::uint32_t min_vertex = static_cast<std::uint32_t>(std::min(particle_a, particle_b));
	const std::uint32_t max_vertex = static_cast<std::uint32_t>(std::max(particle_a, particle_b));
	return (static_cast<std::uint64_t>(min_vertex) << 32) | static_cast<std::uint64_t>(max_vertex);
}

ClothSimulationComponentBase::SpatialHashCell make_spatial_hash_cell(const glm::vec3& position, float cell_size)
{
	const glm::vec3 scaled_position = glm::floor(position / cell_size);
	return {
		static_cast<int>(scaled_position.x),
		static_cast<int>(scaled_position.y),
		static_cast<int>(scaled_position.z),
	};
}
}

std::size_t ClothSimulationComponentBase::SpatialHashCellHasher::operator()(const SpatialHashCell& cell) const
{
	const std::size_t x = std::hash<int>{}(cell[0]);
	const std::size_t y = std::hash<int>{}(cell[1]);
	const std::size_t z = std::hash<int>{}(cell[2]);

	std::size_t seed = x;
	seed ^= y + 0x9e3779b9u + (seed << 6) + (seed >> 2);
	seed ^= z + 0x9e3779b9u + (seed << 6) + (seed >> 2);
	return seed;
}

ClothSimulationComponentBase::ClothSimulationComponentBase(Cloth& cloth)
	: cloth_(cloth)
{
	set_display_name("Cloth Simulation");
}

void ClothSimulationComponentBase::start()
{
	rebuild_constraints();
}

int ClothSimulationComponentBase::solver_pass_count() const
{
	return std::max(1, substeps()) * std::max(1, constraint_iterations());
}

float ClothSimulationComponentBase::max_self_collision_displacement_per_pass() const
{
	if (!self_collision_enabled())
	{
		return 0.0f;
	}

	return 0.2f * collision_margin();
}

void ClothSimulationComponentBase::integrate(float delta_time)
{
	auto& particles = cloth_.get_particles();
	const glm::vec3 acceleration = delta_time * delta_time * (gravity() + external_acceleration_);
	const float damping_per_substep =
		std::pow(std::clamp(damping(), 0.0f, 1.0f), 1.0f / static_cast<float>(solver_pass_count()));
	const float max_displacement = max_self_collision_displacement_per_pass();

	for (Particle& particle : particles)
	{
		if (particle.is_fixed)
		{
			particle.prev_position = particle.position;
			continue;
		}

		glm::vec3 displacement = (particle.position - particle.prev_position) * damping_per_substep + acceleration;
		if (max_displacement > kConstraintLengthThreshold)
		{
			const float displacement_length = glm::length(displacement);
			if (displacement_length > max_displacement)
			{
				displacement *= max_displacement / displacement_length;
			}
		}

		particle.prev_position = particle.position;
		particle.position += displacement;
	}
}

bool ClothSimulationComponentBase::rebuild_constraints()
{
	distance_constraints_.clear();
	rest_positions_.clear();
	self_collision_candidate_ids_.clear();
	cached_topology_revision_ = cloth_.get_topology_revision();

	const auto& particles = cloth_.get_particles();
	const auto& indices = cloth_.get_indices();

	if (particles.empty() || indices.size() < 3 || indices.size() % 3 != 0)
	{
		for (const Particle& particle : particles)
		{
			rest_positions_.push_back(particle.position);
		}
		self_collision_candidate_ids_.assign(particles.size(), {});
		return false;
	}

	rest_positions_.reserve(particles.size());
	std::unordered_set<std::uint64_t> added_distance_constraint_keys;
	added_distance_constraint_keys.reserve(indices.size() * 2);

	const auto add_distance_constraint = [&](int particle_a, int particle_b, DistanceConstraint::Kind kind)
	{
		if (particle_a < 0 || particle_b < 0
			|| particle_a >= static_cast<int>(particles.size())
			|| particle_b >= static_cast<int>(particles.size())
			|| particle_a == particle_b)
		{
			return;
		}

		const std::uint64_t key = make_edge_key(particle_a, particle_b);
		if (!added_distance_constraint_keys.insert(key).second)
		{
			return;
		}

		const float rest_length = glm::length(particles[particle_a].position - particles[particle_b].position);
		distance_constraints_.push_back({ particle_a, particle_b, rest_length, kind });
	};

	const int grid_width = cloth_.get_width();
	const int grid_height = cloth_.get_height();
	const bool has_grid_layout =
		grid_width > 0
		&& grid_height > 0
		&& static_cast<std::size_t>(grid_width * grid_height) == particles.size();

	if (has_grid_layout)
	{
		struct GridConstraintPattern
		{
			int offset_ax = 0;
			int offset_ay = 0;
			int offset_bx = 0;
			int offset_by = 0;
			DistanceConstraint::Kind kind = DistanceConstraint::Kind::Stretch;
		};

		const GridConstraintPattern patterns[] = {
			{ 0, 0, 0, 1, DistanceConstraint::Kind::Stretch },
			{ 0, 0, 1, 0, DistanceConstraint::Kind::Stretch },
			{ 0, 0, 1, 1, DistanceConstraint::Kind::Shear },
			{ 0, 1, 1, 0, DistanceConstraint::Kind::Shear },
			{ 0, 0, 0, 2, DistanceConstraint::Kind::Bend },
			{ 0, 0, 2, 0, DistanceConstraint::Kind::Bend },
		};

		const auto grid_index = [&](int x, int y)
		{
			return y * grid_width + x;
		};

		for (const GridConstraintPattern& pattern : patterns)
		{
			for (int y = 0; y < grid_height; ++y)
			{
				for (int x = 0; x < grid_width; ++x)
				{
					const int ax = x + pattern.offset_ax;
					const int ay = y + pattern.offset_ay;
					const int bx = x + pattern.offset_bx;
					const int by = y + pattern.offset_by;
					if (ax < 0 || ay < 0 || bx < 0 || by < 0
						|| ax >= grid_width || ay >= grid_height
						|| bx >= grid_width || by >= grid_height)
					{
						continue;
					}

					add_distance_constraint(grid_index(ax, ay), grid_index(bx, by), pattern.kind);
				}
			}
		}
	}
	else
	{
		struct EdgeInfo
		{
			int edge_a = 0;
			int edge_b = 0;
			int opposite = 0;
		};

		std::unordered_map<std::uint64_t, EdgeInfo> first_triangle_per_edge;
		first_triangle_per_edge.reserve(indices.size());

		const auto process_shared_edge = [&](int edge_a, int edge_b, int opposite)
		{
			const std::uint64_t key = make_edge_key(edge_a, edge_b);
			const auto [it, inserted] = first_triangle_per_edge.emplace(key, EdgeInfo{ edge_a, edge_b, opposite });
			if (inserted || it->second.opposite == opposite)
			{
				return;
			}

			const EdgeInfo& first = it->second;
			const float shared_edge_length = glm::length(particles[first.edge_a].position - particles[first.edge_b].position);
			const float opposite_distance = glm::length(particles[first.opposite].position - particles[opposite].position);
			const DistanceConstraint::Kind kind =
				shared_edge_length > kConstraintLengthThreshold && opposite_distance <= 1.5f * shared_edge_length
				? DistanceConstraint::Kind::Shear
				: DistanceConstraint::Kind::Bend;

			add_distance_constraint(first.opposite, opposite, kind);
		};

		for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
		{
			const int particle_0 = static_cast<int>(indices[i]);
			const int particle_1 = static_cast<int>(indices[i + 1]);
			const int particle_2 = static_cast<int>(indices[i + 2]);

			if (particle_0 < 0 || particle_1 < 0 || particle_2 < 0
				|| particle_0 >= static_cast<int>(particles.size())
				|| particle_1 >= static_cast<int>(particles.size())
				|| particle_2 >= static_cast<int>(particles.size()))
			{
				continue;
			}

			add_distance_constraint(particle_0, particle_1, DistanceConstraint::Kind::Stretch);
			add_distance_constraint(particle_1, particle_2, DistanceConstraint::Kind::Stretch);
			add_distance_constraint(particle_2, particle_0, DistanceConstraint::Kind::Stretch);

			process_shared_edge(particle_0, particle_1, particle_2);
			process_shared_edge(particle_1, particle_2, particle_0);
			process_shared_edge(particle_2, particle_0, particle_1);
		}
	}

	for (const Particle& particle : particles)
	{
		rest_positions_.push_back(particle.position);
	}

	self_collision_candidate_ids_.assign(particles.size(), {});
	return true;
}

bool ClothSimulationComponentBase::rebuild_constraints_if_needed()
{
	if (cached_topology_revision_ == cloth_.get_topology_revision())
	{
		return false;
	}

	return rebuild_constraints();
}

bool ClothSimulationComponentBase::evaluate_distance_constraint(
	const DistanceConstraint& constraint,
	DistanceConstraintEvaluation& evaluation) const
{
	const auto& particles = cloth_.get_particles();
	const Particle& particle_a = particles[constraint.particle_a];
	const Particle& particle_b = particles[constraint.particle_b];

	if (particle_a.is_fixed && particle_b.is_fixed)
	{
		return false;
	}

	const glm::vec3 delta = particle_a.position - particle_b.position;
	const float current_length = glm::length(delta);
	if (current_length <= kConstraintLengthThreshold)
	{
		return false;
	}

	evaluation.inverse_mass_a = particle_a.is_fixed ? 0.0f : 1.0f;
	evaluation.inverse_mass_b = particle_b.is_fixed ? 0.0f : 1.0f;
	evaluation.denominator = evaluation.inverse_mass_a + evaluation.inverse_mass_b;
	if (evaluation.denominator <= 0.0f)
	{
		return false;
	}

	evaluation.gradient = delta / current_length;
	evaluation.constraint_value = current_length - constraint.rest_length;
	return true;
}

float ClothSimulationComponentBase::per_iteration_stiffness(float stiffness) const
{
	const int total_solver_applications = solver_pass_count();
	if (total_solver_applications <= 0)
	{
		return 0.0f;
	}

	const float clamped_stiffness = std::clamp(stiffness, 0.0f, 1.0f);
	if (clamped_stiffness <= 0.0f || clamped_stiffness >= 1.0f)
	{
		return clamped_stiffness;
	}

	return 1.0f - std::pow(1.0f - clamped_stiffness, 1.0f / static_cast<float>(total_solver_applications));
}

void ClothSimulationComponentBase::build_spatial_hash()
{
	spatial_hash_.clear();

	if (!self_collision_enabled())
	{
		return;
	}

	const auto& particles = cloth_.get_particles();
	if (particles.empty())
	{
		return;
	}

	const float cell_size = collision_margin();
	if (cell_size <= kConstraintLengthThreshold)
	{
		return;
	}

	spatial_hash_.reserve(particles.size());

	for (std::size_t i = 0; i < particles.size(); ++i)
	{
		const SpatialHashCell cell = make_spatial_hash_cell(particles[i].position, cell_size);
		spatial_hash_[cell].push_back(static_cast<unsigned int>(i));
	}
}

void ClothSimulationComponentBase::prepare_self_collision_candidates(float frame_delta_time)
{
	static_cast<void>(frame_delta_time);

	self_collision_candidate_ids_.clear();

	const auto& particles = cloth_.get_particles();
	self_collision_candidate_ids_.resize(particles.size());
	if (!self_collision_enabled() || particles.empty() || rest_positions_.size() != particles.size())
	{
		return;
	}

	build_spatial_hash();
	if (spatial_hash_.empty())
	{
		return;
	}

	const float collision_distance = collision_margin();
	const float max_travel_distance =
		max_self_collision_displacement_per_pass() * static_cast<float>(solver_pass_count());
	const float max_travel_distance_squared = max_travel_distance * max_travel_distance;

	for (std::size_t particle_index = 0; particle_index < particles.size(); ++particle_index)
	{
		const Particle& particle = particles[particle_index];
		const SpatialHashCell min_cell = make_spatial_hash_cell(
			particle.position - glm::vec3(max_travel_distance),
			collision_distance);
		const SpatialHashCell max_cell = make_spatial_hash_cell(
			particle.position + glm::vec3(max_travel_distance),
			collision_distance);

		auto& candidates = self_collision_candidate_ids_[particle_index];
		for (int z = min_cell[2]; z <= max_cell[2]; ++z)
		{
			for (int y = min_cell[1]; y <= max_cell[1]; ++y)
			{
				for (int x = min_cell[0]; x <= max_cell[0]; ++x)
				{
					const SpatialHashCell query_cell = { x, y, z };
					const auto bucket_it = spatial_hash_.find(query_cell);
					if (bucket_it == spatial_hash_.end())
					{
						continue;
					}

					for (const unsigned int candidate_index_unsigned : bucket_it->second)
					{
						const std::size_t candidate_index = static_cast<std::size_t>(candidate_index_unsigned);
						if (candidate_index >= particle_index)
						{
							continue;
						}

						const glm::vec3 delta = particle.position - particles[candidate_index].position;
						if (glm::dot(delta, delta) > max_travel_distance_squared)
						{
							continue;
						}

						candidates.push_back(candidate_index_unsigned);
					}
				}
			}
		}
	}
}

void ClothSimulationComponentBase::solve_collision_objects()
{
	if (!external_collision_enabled())
	{
		return;
	}

	WorldObject* object_owner = owner();
	if (!object_owner || !object_owner->world())
	{
		return;
	}

	const std::vector<CollisionObject*> collision_objects = object_owner->world()->get_objects<CollisionObject>();
	if (collision_objects.empty())
	{
		return;
	}

	auto& particles = cloth_.get_particles();

	for (Particle& particle : particles)
	{
		if (particle.is_fixed)
		{
			continue;
		}

		glm::vec3 world_position = local_to_world_point(particle.position);
		glm::vec3 world_prev_position = local_to_world_point(particle.prev_position);
		bool collided = false;

		for (const CollisionObject* collision_object : collision_objects)
		{
			if (!collision_object)
			{
				continue;
			}

			collided =
				collision_object->resolve_particle_collision(world_position, world_prev_position, collision_margin())
				|| collided;
		}

		if (collided)
		{
			particle.position = world_to_local_point(world_position);
			particle.prev_position = world_to_local_point(world_prev_position);
		}
	}
}

void ClothSimulationComponentBase::solve_self_collision()
{
	if (!self_collision_enabled())
	{
		return;
	}

	const float collision_distance = collision_margin();
	if (collision_distance <= kConstraintLengthThreshold || self_collision_candidate_ids_.empty())
	{
		return;
	}

	auto& particles = cloth_.get_particles();
	if (rest_positions_.size() != particles.size())
	{
		return;
	}

	const float collision_distance_squared = collision_distance * collision_distance;

	for (std::size_t particle_index = 0; particle_index < particles.size(); ++particle_index)
	{
		Particle& particle = particles[particle_index];
		const auto& candidates = self_collision_candidate_ids_[particle_index];

		for (const unsigned int neighbor_index_unsigned : candidates)
		{
			const std::size_t neighbor_index = static_cast<std::size_t>(neighbor_index_unsigned);
			Particle& neighbor = particles[neighbor_index];
			const float inverse_mass_a = particle.is_fixed ? 0.0f : 1.0f;
			const float inverse_mass_b = neighbor.is_fixed ? 0.0f : 1.0f;
			const float inverse_mass_sum = inverse_mass_a + inverse_mass_b;
			if (inverse_mass_sum <= 0.0f)
			{
				continue;
			}

			glm::vec3 delta = neighbor.position - particle.position;
			float distance_squared = glm::dot(delta, delta);
			if (distance_squared >= collision_distance_squared)
			{
				continue;
			}

			if (distance_squared <= kConstraintLengthThreshold * kConstraintLengthThreshold)
			{
				delta = neighbor.prev_position - particle.prev_position;
				distance_squared = glm::dot(delta, delta);
				if (distance_squared <= kConstraintLengthThreshold * kConstraintLengthThreshold)
				{
					continue;
				}
			}

			const glm::vec3 rest_delta = rest_positions_[neighbor_index] - rest_positions_[particle_index];
			const float rest_distance_squared = glm::dot(rest_delta, rest_delta);
			if (distance_squared > rest_distance_squared)
			{
				continue;
			}

			float minimum_distance = collision_distance;
			if (rest_distance_squared < collision_distance_squared)
			{
				minimum_distance = std::sqrt(rest_distance_squared);
			}

			const float distance = std::sqrt(distance_squared);
			if (distance >= minimum_distance)
			{
				continue;
			}

			const glm::vec3 normal = delta / distance;
			const glm::vec3 correction = (minimum_distance - distance) * normal;

			particle.position -= (inverse_mass_a / inverse_mass_sum) * correction;
			neighbor.position += (inverse_mass_b / inverse_mass_sum) * correction;
		}
	}
}

glm::vec3 ClothSimulationComponentBase::world_to_local_point(const glm::vec3& world_point) const
{
	if (const WorldObject* object_owner = owner())
	{
		const glm::mat4 inverse_transform = glm::inverse(object_owner->get_object_transform_matrix());
		return glm::vec3(inverse_transform * glm::vec4(world_point, 1.0f));
	}

	return world_point;
}

glm::vec3 ClothSimulationComponentBase::local_to_world_point(const glm::vec3& local_point) const
{
	if (const WorldObject* object_owner = owner())
	{
		return glm::vec3(object_owner->get_object_transform_matrix() * glm::vec4(local_point, 1.0f));
	}

	return local_point;
}
