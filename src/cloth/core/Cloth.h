#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "cloth/core/Particle.h"

class Cloth
{
public:
	Cloth() = default;
	Cloth(int width, int height, float spacing);
	Cloth(std::vector<glm::vec3> vertices, std::vector<unsigned int> indices);

	void build_grid(int width, int height, float spacing);
	void build_from_mesh(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices);
	void set_fixed(int x, int y, bool is_fixed);
	void set_fixed(int index, bool is_fixed);

	const std::vector<Particle>& get_particles() const;
	std::vector<Particle>& get_particles();
	std::vector<glm::vec3> get_positions() const;
	const std::vector<unsigned int>& get_indices() const;
	std::uint64_t get_topology_revision() const { return topology_revision_; }

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
	std::uint64_t topology_revision_ = 0;
};
