#include "cloth/components/XPBDClothSimulationComponent.h"

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"

#include <algorithm>
#include <cstdint>
#include <glm/geometric.hpp>
#include <unordered_map>

namespace
{
constexpr float kConstraintLengthThreshold = 0.000001f;

std::uint64_t make_edge_key(int particle_a, int particle_b)
{
	const std::uint32_t min_vertex = static_cast<std::uint32_t>(std::min(particle_a, particle_b));
	const std::uint32_t max_vertex = static_cast<std::uint32_t>(std::max(particle_a, particle_b));
	return (static_cast<std::uint64_t>(min_vertex) << 32) | static_cast<std::uint64_t>(max_vertex);
}
}

XPBDClothSimulationComponent::XPBDClothSimulationComponent(Cloth& cloth)
	: cloth_(cloth)
{
}

void XPBDClothSimulationComponent::start()
{
	rebuild_constraints();
}

void XPBDClothSimulationComponent::update_simulation(float delta_time)
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	rebuild_constraints_if_needed();
	integrate(delta_time);
	reset_constraint_lambdas();

	const int iterations = std::max(0, constraint_iterations_);
	for (int i = 0; i < iterations; ++i)
	{
		solve_distance_constraints(delta_time);
	}
}

void XPBDClothSimulationComponent::integrate(float delta_time)
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

void XPBDClothSimulationComponent::rebuild_constraints()
{
	distance_constraints_.clear();
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
	distance_constraints_.reserve(indices.size() * 2);

	const auto add_edge = [&](int edge_a, int edge_b, int opposite)
	{
		const std::uint64_t key = make_edge_key(edge_a, edge_b);
		const auto [it, inserted] = first_triangle_per_edge.emplace(key, EdgeInfo{ edge_a, edge_b, opposite });

		if (inserted)
		{
			const float rest_length = glm::length(particles[edge_a].position - particles[edge_b].position);
			distance_constraints_.push_back({ edge_a, edge_b, rest_length, stretch_compliance_, 0.0f });
			return;
		}

		const EdgeInfo& first = it->second;
		if (first.opposite == opposite)
		{
			return;
		}

		const float bend_rest_length = glm::length(particles[first.opposite].position - particles[opposite].position);
		distance_constraints_.push_back({ first.opposite, opposite, bend_rest_length, bend_compliance_, 0.0f });
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

void XPBDClothSimulationComponent::rebuild_constraints_if_needed()
{
	if (cached_topology_revision_ != cloth_.get_topology_revision())
	{
		rebuild_constraints();
	}
}

void XPBDClothSimulationComponent::reset_constraint_lambdas()
{
	for (auto& constraint : distance_constraints_)
	{
		constraint.lambda = 0.0f;
	}
}

void XPBDClothSimulationComponent::solve_distance_constraints(float delta_time)
{
	for (auto& constraint : distance_constraints_)
	{
		solve_distance_constraint(constraint, delta_time);
	}
}

void XPBDClothSimulationComponent::solve_distance_constraint(DistanceConstraint& constraint, float delta_time)
{
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
	const float alpha_tilde = constraint.compliance / (delta_time * delta_time);
	const float denominator = inverse_mass_sum + alpha_tilde;

	if (denominator <= 0.0f)
	{
		return;
	}

	const float delta_lambda = (-constraint_value - alpha_tilde * constraint.lambda) / denominator;
	const glm::vec3 correction = delta_lambda * gradient;

	constraint.lambda += delta_lambda;
	particle_a.position += inverse_mass_a * correction;
	particle_b.position -= inverse_mass_b * correction;
}
