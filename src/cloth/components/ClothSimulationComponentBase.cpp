#include "cloth/components/ClothSimulationComponentBase.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"
#include "engine/core/Object.h"
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

float cot_theta(const glm::vec3& first, const glm::vec3& second)
{
	const float sin_theta = glm::length(glm::cross(first, second));
	if (sin_theta <= kConstraintLengthThreshold)
	{
		return 0.0f;
	}

	return glm::dot(first, second) / sin_theta;
}

float& matrix_entry(std::array<float, 16>& matrix, int row, int column)
{
	return matrix[static_cast<std::size_t>(row * 4 + column)];
}

float matrix_entry(const std::array<float, 16>& matrix, int row, int column)
{
	return matrix[static_cast<std::size_t>(row * 4 + column)];
}

bool is_valid_q_matrix(const std::array<float, 16>& q_matrix)
{
	for (const float value : q_matrix)
	{
		if (!std::isfinite(value))
		{
			return false;
		}
	}

	return true;
}

bool build_isometric_bending_matrix(
	const glm::vec3& particle_1,
	const glm::vec3& particle_2,
	const glm::vec3& particle_3,
	const glm::vec3& particle_4,
	std::array<float, 16>& q_matrix)
{
	q_matrix.fill(0.0f);

	const glm::vec3 vertices[4] = {
		particle_3,
		particle_4,
		particle_1,
		particle_2,
	};

	const glm::vec3 edge_0 = vertices[1] - vertices[0];
	const glm::vec3 edge_1 = vertices[2] - vertices[0];
	const glm::vec3 edge_2 = vertices[3] - vertices[0];
	const glm::vec3 edge_3 = vertices[2] - vertices[1];
	const glm::vec3 edge_4 = vertices[3] - vertices[1];

	const float c01 = cot_theta(edge_0, edge_1);
	const float c02 = cot_theta(edge_0, edge_2);
	const float c03 = cot_theta(-edge_0, edge_3);
	const float c04 = cot_theta(-edge_0, edge_4);

	const float area_0 = 0.5f * glm::length(glm::cross(edge_0, edge_1));
	const float area_1 = 0.5f * glm::length(glm::cross(edge_0, edge_2));
	const float area_sum = area_0 + area_1;
	if (area_sum <= kConstraintLengthThreshold)
	{
		return false;
	}

	const float coefficient = -3.0f / (2.0f * area_sum);
	const float k_values[4] = {
		c03 + c04,
		c01 + c02,
		-c01 - c03,
		-c02 - c04,
	};
	const float scaled_k_values[4] = {
		coefficient * k_values[0],
		coefficient * k_values[1],
		coefficient * k_values[2],
		coefficient * k_values[3],
	};

	for (int row = 0; row < 4; ++row)
	{
		for (int column = 0; column < row; ++column)
		{
			const float value = k_values[row] * scaled_k_values[column];
			matrix_entry(q_matrix, row, column) = value;
			matrix_entry(q_matrix, column, row) = value;
		}

		matrix_entry(q_matrix, row, row) = k_values[row] * scaled_k_values[row];
	}

	return is_valid_q_matrix(q_matrix);
}

}

ClothSimulationComponentBase::ClothSimulationComponentBase(Cloth& cloth)
	: cloth_(cloth)
{
}

void ClothSimulationComponentBase::start()
{
	rebuild_constraints();
}

void ClothSimulationComponentBase::integrate(float delta_time)
{
	auto& particles = cloth_.get_particles();
	const glm::vec3 acceleration = delta_time * delta_time * (gravity_ + external_acceleration_);
	external_acceleration_ = glm::vec3(0.0f);

	for (Particle& particle : particles)
	{
		if (particle.is_fixed)
		{
			particle.prev_position = particle.position;
			continue;
		}

		const glm::vec3 velocity = (particle.position - particle.prev_position) * damping_;
		particle.prev_position = particle.position;
		particle.position += velocity + acceleration;
	}
}

bool ClothSimulationComponentBase::rebuild_constraints()
{
	distance_constraints_.clear();
	bending_constraints_.clear();
	cached_topology_revision_ = cloth_.get_topology_revision();

	const auto& particles = cloth_.get_particles();
	const auto& indices = cloth_.get_indices();

	if (particles.empty() || indices.size() < 3 || indices.size() % 3 != 0)
	{
		return false;
	}

	struct EdgeInfo
	{
		int edge_a = 0;
		int edge_b = 0;
		int opposite = 0;
	};

	std::unordered_map<std::uint64_t, EdgeInfo> first_triangle_per_edge;
	first_triangle_per_edge.reserve(indices.size());
	distance_constraints_.reserve(indices.size());
	bending_constraints_.reserve(indices.size() / 3);

	const auto add_edge = [&](int edge_a, int edge_b, int opposite)
	{
		const std::uint64_t key = make_edge_key(edge_a, edge_b);
		const auto [it, inserted] = first_triangle_per_edge.emplace(key, EdgeInfo{ edge_a, edge_b, opposite });

		if (inserted)
		{
			const float rest_length = glm::length(particles[edge_a].position - particles[edge_b].position);
			distance_constraints_.push_back({ edge_a, edge_b, rest_length });
			return;
		}

		const EdgeInfo& first = it->second;
		if (first.opposite == opposite)
		{
			return;
		}

		std::array<float, 16> q_matrix;
		if (!build_isometric_bending_matrix(
			particles[first.opposite].position,
			particles[opposite].position,
			particles[first.edge_a].position,
			particles[first.edge_b].position,
			q_matrix))
		{
			return;
		}

		bending_constraints_.push_back({ first.opposite, opposite, first.edge_a, first.edge_b, q_matrix });
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

		add_edge(particle_0, particle_1, particle_2);
		add_edge(particle_1, particle_2, particle_0);
		add_edge(particle_2, particle_0, particle_1);
	}

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

bool ClothSimulationComponentBase::evaluate_bending_constraint(
	const BendingConstraint& constraint,
	BendingConstraintEvaluation& evaluation) const
{
	const auto& particles = cloth_.get_particles();
	const Particle& particle_1 = particles[constraint.particle_1];
	const Particle& particle_2 = particles[constraint.particle_2];
	const Particle& particle_3 = particles[constraint.particle_3];
	const Particle& particle_4 = particles[constraint.particle_4];

	evaluation.inverse_mass_1 = particle_1.is_fixed ? 0.0f : 1.0f;
	evaluation.inverse_mass_2 = particle_2.is_fixed ? 0.0f : 1.0f;
	evaluation.inverse_mass_3 = particle_3.is_fixed ? 0.0f : 1.0f;
	evaluation.inverse_mass_4 = particle_4.is_fixed ? 0.0f : 1.0f;

	if (evaluation.inverse_mass_1 + evaluation.inverse_mass_2 + evaluation.inverse_mass_3 + evaluation.inverse_mass_4 <= 0.0f)
	{
		return false;
	}

	const glm::vec3 stencil_positions[4] = {
		particle_3.position,
		particle_4.position,
		particle_1.position,
		particle_2.position,
	};
	const float inverse_masses[4] = {
		evaluation.inverse_mass_3,
		evaluation.inverse_mass_4,
		evaluation.inverse_mass_1,
		evaluation.inverse_mass_2,
	};

	evaluation.constraint_value = 0.0f;
	for (int column = 0; column < 4; ++column)
	{
		for (int row = 0; row < 4; ++row)
		{
			evaluation.constraint_value +=
				matrix_entry(constraint.q_matrix, row, column) *
				glm::dot(stencil_positions[column], stencil_positions[row]);
		}
	}
	evaluation.constraint_value *= 0.5f;

	if (std::abs(evaluation.constraint_value) <= kConstraintLengthThreshold)
	{
		return false;
	}

	glm::vec3 gradients[4] = {
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
	};

	for (int column = 0; column < 4; ++column)
	{
		for (int row = 0; row < 4; ++row)
		{
			gradients[row] += matrix_entry(constraint.q_matrix, row, column) * stencil_positions[column];
		}
	}

	evaluation.q1 = gradients[2];
	evaluation.q2 = gradients[3];
	evaluation.q3 = gradients[0];
	evaluation.q4 = gradients[1];

	const float weighted_q_norm =
		inverse_masses[0] * glm::dot(gradients[0], gradients[0]) +
		inverse_masses[1] * glm::dot(gradients[1], gradients[1]) +
		inverse_masses[2] * glm::dot(gradients[2], gradients[2]) +
		inverse_masses[3] * glm::dot(gradients[3], gradients[3]);
	if (weighted_q_norm <= kConstraintLengthThreshold)
	{
		return false;
	}

	evaluation.denominator = weighted_q_norm;
	return evaluation.denominator > kConstraintLengthThreshold;
}

float ClothSimulationComponentBase::per_iteration_stiffness(float stiffness) const
{
	if (constraint_iterations_ <= 0)
	{
		return 0.0f;
	}

	const float clamped_stiffness = std::clamp(stiffness, 0.0f, 1.0f);
	if (clamped_stiffness <= 0.0f || clamped_stiffness >= 1.0f)
	{
		return clamped_stiffness;
	}

	return 1.0f - std::pow(1.0f - clamped_stiffness, 1.0f / static_cast<float>(constraint_iterations_));
}

void ClothSimulationComponentBase::solve_collision_objects()
{
	if (!collision_enabled_)
	{
		return;
	}

	Object* object_owner = owner();
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
				collision_object->resolve_particle_collision(world_position, world_prev_position, collision_margin_)
				|| collided;
		}

		if (collided)
		{
			particle.position = world_to_local_point(world_position);
			particle.prev_position = world_to_local_point(world_prev_position);
		}
	}
}

glm::vec3 ClothSimulationComponentBase::world_to_local_point(const glm::vec3& world_point) const
{
	if (const Object* object_owner = owner())
	{
		const glm::mat4 inverse_transform = glm::inverse(object_owner->transform().matrix());
		return glm::vec3(inverse_transform * glm::vec4(world_point, 1.0f));
	}

	return world_point;
}

glm::vec3 ClothSimulationComponentBase::local_to_world_point(const glm::vec3& local_point) const
{
	if (const Object* object_owner = owner())
	{
		return glm::vec3(object_owner->transform().matrix() * glm::vec4(local_point, 1.0f));
	}

	return local_point;
}
