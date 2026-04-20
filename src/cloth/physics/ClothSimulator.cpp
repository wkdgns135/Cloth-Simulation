#include "cloth/physics/ClothSimulator.h"

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"

#include <algorithm>
#include <glm/geometric.hpp>

void ClothSimulator::step(Cloth& cloth, float delta_time) const
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	integrate(cloth, delta_time);

	const int iterations = std::max(0, constraint_iterations_);
	for (int i = 0; i < iterations; ++i)
	{
		solve_structural_constraints(cloth);
	}
}

void ClothSimulator::integrate(Cloth& cloth, float delta_time) const
{
	auto& particles = cloth.get_particles();
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

void ClothSimulator::solve_structural_constraints(Cloth& cloth) const
{
	auto& particles = cloth.get_particles();
	const int width = cloth.get_width();
	const int height = cloth.get_height();
	const float spacing = cloth.get_spacing();

	if (width <= 1 || height <= 1 || spacing <= 0.0f)
	{
		return;
	}

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const int index = y * width + x;

			if (x + 1 < width)
			{
				satisfy_distance(particles[index], particles[index + 1], spacing);
			}

			if (y + 1 < height)
			{
				satisfy_distance(particles[index], particles[index + width], spacing);
			}
		}
	}
}

void ClothSimulator::satisfy_distance(Particle& a, Particle& b, float rest_length) const
{
	if (a.is_fixed && b.is_fixed)
	{
		return;
	}

	const glm::vec3 delta = b.position - a.position;
	const float current_length = glm::length(delta);

	if (current_length <= 0.000001f)
	{
		return;
	}

	const glm::vec3 correction = delta * ((current_length - rest_length) / current_length);

	if (a.is_fixed)
	{
		b.position -= correction;
		return;
	}

	if (b.is_fixed)
	{
		a.position += correction;
		return;
	}

	a.position += correction * 0.5f;
	b.position -= correction * 0.5f;
}
