#include "cloth/components/ClothSimulationComponent.h"

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"

#include <algorithm>
#include <glm/geometric.hpp>

namespace
{
constexpr float kConstraintLengthThreshold = 0.000001f;
}

ClothSimulationComponent::ClothSimulationComponent(Cloth& cloth)
	: cloth_(cloth)
{
}

void ClothSimulationComponent::start()
{
	rebuild_constraints();
}

void ClothSimulationComponent::update_simulation(float delta_time)
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	rebuild_constraints_if_needed();
	integrate(delta_time);
	prepare_distance_constraints(delta_time);

	const int iterations = std::max(0, constraint_iterations_);
	for (int i = 0; i < iterations; ++i)
	{
		solve_distance_constraints(delta_time);
	}
}

void ClothSimulationComponent::integrate(float delta_time)
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

void ClothSimulationComponent::rebuild_constraints()
{
	const int width = cloth_.get_width();
	const int height = cloth_.get_height();
	const float spacing = cloth_.get_spacing();

	distance_constraints_.clear();
	cached_width_ = width;
	cached_height_ = height;
	cached_spacing_ = spacing;

	if (width <= 1 || height <= 1 || spacing <= 0.0f)
	{
		return;
	}

	distance_constraints_.reserve((width - 1) * height + width * (height - 1));

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const int index = y * width + x;

			if (x + 1 < width)
			{
				distance_constraints_.push_back({ index, index + 1, spacing, stretch_compliance_, 0.0f });
			}

			if (y + 1 < height)
			{
				distance_constraints_.push_back({ index, index + width, spacing, stretch_compliance_, 0.0f });
			}
		}
	}
}

void ClothSimulationComponent::rebuild_constraints_if_needed()
{
	if (cached_width_ != cloth_.get_width()
		|| cached_height_ != cloth_.get_height()
		|| cached_spacing_ != cloth_.get_spacing())
	{
		rebuild_constraints();
	}
}

void ClothSimulationComponent::prepare_distance_constraints(float)
{
}

void ClothSimulationComponent::reset_constraint_lambdas()
{
	for (auto& constraint : distance_constraints_)
	{
		constraint.lambda = 0.0f;
	}
}

void ClothSimulationComponent::solve_distance_constraints(float delta_time)
{
	for (auto& constraint : distance_constraints_)
	{
		solve_distance_constraint(constraint, delta_time);
	}
}

void ClothSimulationComponent::solve_distance_constraint(DistanceConstraint& constraint, float delta_time)
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

	DistanceConstraintSolveData solve_data{
		particle_a,
		particle_b,
		delta / current_length,
		current_length - constraint.rest_length,
		inverse_mass_a,
		inverse_mass_b,
		inverse_mass_sum
	};

	const float delta_lambda = compute_distance_delta_lambda(constraint, solve_data, delta_time);
	const glm::vec3 correction = delta_lambda * solve_data.gradient;

	solve_data.particle_a.position += solve_data.inverse_mass_a * correction;
	solve_data.particle_b.position -= solve_data.inverse_mass_b * correction;
}
