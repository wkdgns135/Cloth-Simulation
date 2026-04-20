#include "cloth/core/Cloth.h"

Cloth::Cloth(int width, int height, float spacing)
{
	build_grid(width, height, spacing);
}

void Cloth::build_grid(int width, int height, float spacing)
{
	width_ = width;
	height_ = height;
	spacing_ = spacing;

	particles_.clear();
	particles_.reserve(width_ * height_);

	const float start_x = -(width_ - 1) * spacing_ * 0.5f;
	const float start_y = (height_ - 1) * spacing_ * 0.5f;

	for (int y = 0; y < height_; ++y)
	{
		for (int x = 0; x < width_; ++x)
		{
			const float px = start_x + x * spacing_;
			const float py = start_y - y * spacing_;

			particles_.emplace_back(glm::vec3(px, py, 0.0f));
		}
	}
}

void Cloth::set_fixed(int x, int y, bool is_fixed)
{
	if (x < 0 || x >= width_ || y < 0 || y >= height_)
	{
		return;
	}

	particles_[get_index(x, y)].is_fixed = is_fixed;
}

const std::vector<Particle>& Cloth::get_particles() const
{
	return particles_;
}

std::vector<Particle>& Cloth::get_particles()
{
	return particles_;
}

std::vector<glm::vec3> Cloth::get_positions() const
{
	std::vector<glm::vec3> positions;
	positions.reserve(particles_.size());

	for (const auto& particle : particles_)
	{
		positions.push_back(particle.position);
	}

	return positions;
}

int Cloth::get_index(int x, int y) const
{
	return y * width_ + x;
}
