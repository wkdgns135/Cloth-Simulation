#include "cloth/components/PBDClothSimulationComponent.h"

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/geometric.hpp>
#include <unordered_map>

namespace
{
constexpr float kConstraintLengthThreshold = 0.000001f;
constexpr float kBendingDotClampEpsilon = 0.0001f;

std::uint64_t make_edge_key(int particle_a, int particle_b)
{
	const std::uint32_t min_vertex = static_cast<std::uint32_t>(std::min(particle_a, particle_b));
	const std::uint32_t max_vertex = static_cast<std::uint32_t>(std::max(particle_a, particle_b));
	return (static_cast<std::uint64_t>(min_vertex) << 32) | static_cast<std::uint64_t>(max_vertex);
}

float compute_dihedral_angle(
	const glm::vec3& particle_1,
	const glm::vec3& particle_2,
	const glm::vec3& particle_3,
	const glm::vec3& particle_4)
{
	const glm::vec3 edge = particle_2 - particle_1;
	const glm::vec3 normal_1 = glm::normalize(glm::cross(edge, particle_3 - particle_1));
	const glm::vec3 normal_2 = glm::normalize(glm::cross(edge, particle_4 - particle_1));
	const float dot_value = std::clamp(glm::dot(normal_1, normal_2), -1.0f, 1.0f);
	return std::acos(dot_value);
}
}

PBDClothSimulationComponent::PBDClothSimulationComponent(Cloth& cloth)
	: cloth_(cloth)
{
}

void PBDClothSimulationComponent::start()
{
	rebuild_constraints();
}

void PBDClothSimulationComponent::update_simulation(float delta_time)
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	rebuild_constraints_if_needed();
	integrate(delta_time);

	const int iterations = std::max(0, constraint_iterations_);
	const float stretch_stiffness = per_iteration_stiffness(stretch_stiffness_, iterations);
	const float bend_stiffness = per_iteration_stiffness(bend_stiffness_, iterations);

	for (int i = 0; i < iterations; ++i)
	{
		solve_distance_constraints(stretch_stiffness);
		solve_bending_constraints(bend_stiffness);
	}
}

void PBDClothSimulationComponent::integrate(float delta_time)
{
	auto& particles = cloth_.get_particles();
	const glm::vec3 acceleration = delta_time * delta_time * gravity_;

	for (auto& particle : particles)
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

void PBDClothSimulationComponent::rebuild_constraints()
{
	distance_constraints_.clear();
	bending_constraints_.clear();
	cached_topology_revision_ = cloth_.get_topology_revision();

	const auto& particles = cloth_.get_particles();
	const auto& indices = cloth_.get_indices();

	if (particles.empty() || indices.size() < 3)
	{
		return;
	}

	if (indices.size() % 3 != 0)
	{
		return;
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

		const glm::vec3 rest_1 = particles[first.edge_a].position;
		const glm::vec3 rest_2 = particles[first.edge_b].position;
		const glm::vec3 rest_3 = particles[first.opposite].position;
		const glm::vec3 rest_4 = particles[opposite].position;
		const float rest_angle = compute_dihedral_angle(rest_1, rest_2, rest_3, rest_4);

		bending_constraints_.push_back({ first.edge_a, first.edge_b, first.opposite, opposite, rest_angle });
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
}

void PBDClothSimulationComponent::rebuild_constraints_if_needed()
{
	if (cached_topology_revision_ != cloth_.get_topology_revision())
	{
		rebuild_constraints();
	}
}

void PBDClothSimulationComponent::solve_distance_constraints(float stiffness_per_iteration)
{
	for (const auto& constraint : distance_constraints_)
	{
		solve_distance_constraint(constraint, stiffness_per_iteration);
	}
}

void PBDClothSimulationComponent::solve_distance_constraint(const DistanceConstraint& constraint, float stiffness_per_iteration)
{
	if (stiffness_per_iteration <= 0.0f)
	{
		return;
	}

	auto& particles = cloth_.get_particles();
	Particle& particle_a = particles[constraint.particle_a];
	Particle& particle_b = particles[constraint.particle_b];

	if (particle_a.is_fixed && particle_b.is_fixed)
	{
		return;
	}

	const glm::vec3 delta = particle_a.position - particle_b.position;
	const float current_length = glm::length(delta);

	if (current_length <= kConstraintLengthThreshold)
	{
		return;
	}

	const float inverse_mass_a = particle_a.is_fixed ? 0.0f : 1.0f;
	const float inverse_mass_b = particle_b.is_fixed ? 0.0f : 1.0f;
	const float inverse_mass_sum = inverse_mass_a + inverse_mass_b;

	if (inverse_mass_sum <= 0.0f)
	{
		return;
	}

	const glm::vec3 gradient = delta / current_length;
	const float constraint_value = current_length - constraint.rest_length;
	const float delta_lambda = -stiffness_per_iteration * constraint_value / inverse_mass_sum;
	const glm::vec3 correction = delta_lambda * gradient;

	particle_a.position += inverse_mass_a * correction;
	particle_b.position -= inverse_mass_b * correction;
}

void PBDClothSimulationComponent::solve_bending_constraints(float stiffness_per_iteration)
{
	for (const auto& constraint : bending_constraints_)
	{
		solve_bending_constraint(constraint, stiffness_per_iteration);
	}
}

void PBDClothSimulationComponent::solve_bending_constraint(const BendingConstraint& constraint, float stiffness_per_iteration)
{
	if (stiffness_per_iteration <= 0.0f)
	{
		return;
	}

	auto& particles = cloth_.get_particles();
	Particle& particle_1 = particles[constraint.particle_1];
	Particle& particle_2 = particles[constraint.particle_2];
	Particle& particle_3 = particles[constraint.particle_3];
	Particle& particle_4 = particles[constraint.particle_4];

	const float inverse_mass_1 = particle_1.is_fixed ? 0.0f : 1.0f;
	const float inverse_mass_2 = particle_2.is_fixed ? 0.0f : 1.0f;
	const float inverse_mass_3 = particle_3.is_fixed ? 0.0f : 1.0f;
	const float inverse_mass_4 = particle_4.is_fixed ? 0.0f : 1.0f;

	if (inverse_mass_1 + inverse_mass_2 + inverse_mass_3 + inverse_mass_4 <= 0.0f)
	{
		return;
	}

	const glm::vec3 p1 = particle_1.position;
	const glm::vec3 p2 = particle_2.position;
	const glm::vec3 p3 = particle_3.position;
	const glm::vec3 p4 = particle_4.position;

	const glm::vec3 r2 = p2 - p1;
	const glm::vec3 r3 = p3 - p1;
	const glm::vec3 r4 = p4 - p1;

	const glm::vec3 cross_23 = glm::cross(r2, r3);
	const glm::vec3 cross_24 = glm::cross(r2, r4);
	const float cross_23_length = glm::length(cross_23);
	const float cross_24_length = glm::length(cross_24);

	if (cross_23_length <= kConstraintLengthThreshold || cross_24_length <= kConstraintLengthThreshold)
	{
		return;
	}

	const glm::vec3 normal_1 = cross_23 / cross_23_length;
	const glm::vec3 normal_2 = cross_24 / cross_24_length;
	const float d = std::clamp(glm::dot(normal_1, normal_2), -1.0f, 1.0f);
	const float d_for_scale = std::clamp(d, -1.0f + kBendingDotClampEpsilon, 1.0f - kBendingDotClampEpsilon);
	const float one_minus_d_squared = std::max(1.0f - d_for_scale * d_for_scale, kBendingDotClampEpsilon);

	const float constraint_value = std::acos(d) - constraint.rest_angle;
	if (std::abs(constraint_value) <= kConstraintLengthThreshold)
	{
		return;
	}

	const glm::vec3 q3 = (glm::cross(r2, normal_2) + glm::cross(normal_1, r2) * d_for_scale) / cross_23_length;
	const glm::vec3 q4 = (glm::cross(r2, normal_1) + glm::cross(normal_2, r2) * d_for_scale) / cross_24_length;
	const glm::vec3 q2 =
		-(glm::cross(r3, normal_2) + glm::cross(normal_1, r3) * d_for_scale) / cross_23_length
		-(glm::cross(r4, normal_1) + glm::cross(normal_2, r4) * d_for_scale) / cross_24_length;
	const glm::vec3 q1 = -q2 - q3 - q4;

	const float denominator =
		inverse_mass_1 * glm::dot(q1, q1) +
		inverse_mass_2 * glm::dot(q2, q2) +
		inverse_mass_3 * glm::dot(q3, q3) +
		inverse_mass_4 * glm::dot(q4, q4);

	if (denominator <= kConstraintLengthThreshold)
	{
		return;
	}

	const float scale = -stiffness_per_iteration * constraint_value / (std::sqrt(one_minus_d_squared) * denominator);

	particle_1.position += inverse_mass_1 * scale * q1;
	particle_2.position += inverse_mass_2 * scale * q2;
	particle_3.position += inverse_mass_3 * scale * q3;
	particle_4.position += inverse_mass_4 * scale * q4;
}

float PBDClothSimulationComponent::per_iteration_stiffness(float stiffness, int iteration_count) const
{
	if (iteration_count <= 0)
	{
		return 0.0f;
	}

	const float clamped_stiffness = std::clamp(stiffness, 0.0f, 1.0f);
	if (clamped_stiffness <= 0.0f || clamped_stiffness >= 1.0f)
	{
		return clamped_stiffness;
	}

	return 1.0f - std::pow(1.0f - clamped_stiffness, 1.0f / static_cast<float>(iteration_count));
}
