#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "cloth/core/Particle.h"

class Cloth
{
public:
	Cloth() = default;
	Cloth(int width, int height, float spacing);

	void build_grid(int width, int height, float spacing);
	void set_fixed(int x, int y, bool is_fixed);

	const std::vector<Particle>& get_particles() const;
	std::vector<Particle>& get_particles();
	std::vector<glm::vec3> get_positions() const;
	const std::vector<unsigned int>& get_indices() const;

	int get_width() const { return width_; }
	int get_height() const { return height_; }
	float get_spacing() const { return spacing_; }

private:
	int get_index(int x, int y) const;

	int width_ = 0;
	int height_ = 0;
	float spacing_ = 0.0f;

	std::vector<Particle> particles_;
	std::vector<unsigned int> indices_;
};
