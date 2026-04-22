#include "cloth/core/Cloth.h"
#include <glm/gtc/random.hpp>

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
			
			particles_.emplace_back(glm::vec3(px, py, glm::linearRand(-1.0f, 1.0f)));
		}
	}

	indices_.clear();
	if (width_ < 2 || height_ < 2)
	{
		return;
	}

	indices_.reserve((width_ - 1) * (height_ - 1) * 6);

	for (int y = 0; y < height_ - 1; ++y)
	{
		for (int x = 0; x < width_ - 1; ++x)
		{
			const unsigned int i0 = static_cast<unsigned int>(get_index(x, y));
			const unsigned int i1 = static_cast<unsigned int>(get_index(x + 1, y));
			const unsigned int i2 = static_cast<unsigned int>(get_index(x, y + 1));
			const unsigned int i3 = static_cast<unsigned int>(get_index(x + 1, y + 1));

			indices_.push_back(i0);
			indices_.push_back(i2);
			indices_.push_back(i1);

			indices_.push_back(i1);
			indices_.push_back(i2);
			indices_.push_back(i3);
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

const std::vector<unsigned int>& Cloth::get_indices() const
{
	return indices_;
}

int Cloth::get_index(int x, int y) const
{
	return y * width_ + x;
}
