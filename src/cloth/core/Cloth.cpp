#include "cloth/core/Cloth.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

#include <glm/geometric.hpp>

namespace
{
constexpr float kGridInferenceEpsilon = 0.0001f;

struct GridCoordinates
{
	int x = 0;
	int y = 0;
};

bool triangle_matches_grid_cell(
	const std::array<GridCoordinates, 3>& triangle_coordinates,
	int& cell_x,
	int& cell_y)
{
	int min_x = triangle_coordinates[0].x;
	int max_x = triangle_coordinates[0].x;
	int min_y = triangle_coordinates[0].y;
	int max_y = triangle_coordinates[0].y;

	for (const GridCoordinates& coordinates : triangle_coordinates)
	{
		min_x = std::min(min_x, coordinates.x);
		max_x = std::max(max_x, coordinates.x);
		min_y = std::min(min_y, coordinates.y);
		max_y = std::max(max_y, coordinates.y);
	}

	if (max_x - min_x != 1 || max_y - min_y != 1)
	{
		return false;
	}

	bool corners[2][2] = {
		{ false, false },
		{ false, false },
	};

	for (const GridCoordinates& coordinates : triangle_coordinates)
	{
		const int local_x = coordinates.x - min_x;
		const int local_y = coordinates.y - min_y;
		if (local_x < 0 || local_x > 1 || local_y < 0 || local_y > 1 || corners[local_y][local_x])
		{
			return false;
		}

		corners[local_y][local_x] = true;
	}

	cell_x = min_x;
	cell_y = min_y;
	return true;
}

bool infer_grid_layout_from_mesh(
	const std::vector<glm::vec3>& vertices,
	const std::vector<unsigned int>& indices,
	int& width,
	int& height,
	float& spacing,
	std::vector<glm::vec3>& reordered_vertices,
	std::vector<unsigned int>& remapped_indices)
{
	width = 0;
	height = 0;
	spacing = 0.0f;
	reordered_vertices.clear();
	remapped_indices.clear();

	if (vertices.size() < 4 || indices.size() < 6 || indices.size() % 3 != 0)
	{
		return false;
	}

	std::vector<unsigned int> sorted_vertex_ids(vertices.size());
	std::iota(sorted_vertex_ids.begin(), sorted_vertex_ids.end(), 0u);
	std::stable_sort(
		sorted_vertex_ids.begin(),
		sorted_vertex_ids.end(),
		[&vertices](unsigned int lhs, unsigned int rhs)
		{
			const glm::vec3& a = vertices[lhs];
			const glm::vec3& b = vertices[rhs];
			if (std::abs(a.y - b.y) > kGridInferenceEpsilon)
			{
				return a.y > b.y;
			}
			if (std::abs(a.x - b.x) > kGridInferenceEpsilon)
			{
				return a.x < b.x;
			}
			return a.z < b.z;
		});

	std::vector<std::vector<unsigned int>> rows;
	for (const unsigned int vertex_id : sorted_vertex_ids)
	{
		if (rows.empty() || std::abs(vertices[vertex_id].y - vertices[rows.back().front()].y) > kGridInferenceEpsilon)
		{
			rows.emplace_back();
		}

		rows.back().push_back(vertex_id);
	}

	height = static_cast<int>(rows.size());
	if (height < 2)
	{
		return false;
	}

	width = static_cast<int>(rows.front().size());
	if (width < 2)
	{
		return false;
	}

	for (std::vector<unsigned int>& row : rows)
	{
		if (static_cast<int>(row.size()) != width)
		{
			return false;
		}

		std::stable_sort(
			row.begin(),
			row.end(),
			[&vertices](unsigned int lhs, unsigned int rhs)
			{
				const glm::vec3& a = vertices[lhs];
				const glm::vec3& b = vertices[rhs];
				if (std::abs(a.x - b.x) > kGridInferenceEpsilon)
				{
					return a.x < b.x;
				}
				return a.z < b.z;
			});
	}

	if (static_cast<std::size_t>(width * height) != vertices.size())
	{
		return false;
	}

	const std::size_t expected_triangle_count = static_cast<std::size_t>(2 * (width - 1) * (height - 1));
	if (indices.size() / 3 != expected_triangle_count)
	{
		return false;
	}

	std::vector<int> remap(vertices.size(), -1);
	std::vector<GridCoordinates> grid_coordinates(vertices.size());
	reordered_vertices.resize(vertices.size());

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const unsigned int source_vertex_id = rows[y][x];
			const int destination_vertex_id = y * width + x;
			remap[source_vertex_id] = destination_vertex_id;
			grid_coordinates[destination_vertex_id] = { x, y };
			reordered_vertices[destination_vertex_id] = vertices[source_vertex_id];
		}
	}

	remapped_indices.resize(indices.size());
	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		const unsigned int source_vertex_id = indices[i];
		if (source_vertex_id >= vertices.size())
		{
			return false;
		}

		const int destination_vertex_id = remap[source_vertex_id];
		if (destination_vertex_id < 0)
		{
			return false;
		}

		remapped_indices[i] = static_cast<unsigned int>(destination_vertex_id);
	}

	std::vector<int> cell_triangle_counts(static_cast<std::size_t>((width - 1) * (height - 1)), 0);
	for (std::size_t i = 0; i + 2 < remapped_indices.size(); i += 3)
	{
		const std::array<GridCoordinates, 3> triangle_coordinates = {
			grid_coordinates[remapped_indices[i]],
			grid_coordinates[remapped_indices[i + 1]],
			grid_coordinates[remapped_indices[i + 2]],
		};

		int cell_x = 0;
		int cell_y = 0;
		if (!triangle_matches_grid_cell(triangle_coordinates, cell_x, cell_y))
		{
			return false;
		}

		++cell_triangle_counts[static_cast<std::size_t>(cell_y * (width - 1) + cell_x)];
	}

	if (std::any_of(cell_triangle_counts.begin(), cell_triangle_counts.end(), [](int count) { return count != 2; }))
	{
		return false;
	}

	float spacing_sum = 0.0f;
	int spacing_count = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x + 1 < width; ++x)
		{
			spacing_sum += glm::length(reordered_vertices[y * width + x + 1] - reordered_vertices[y * width + x]);
			++spacing_count;
		}
	}

	for (int y = 0; y + 1 < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			spacing_sum += glm::length(reordered_vertices[(y + 1) * width + x] - reordered_vertices[y * width + x]);
			++spacing_count;
		}
	}

	if (spacing_count > 0)
	{
		spacing = spacing_sum / static_cast<float>(spacing_count);
	}

	return true;
}
}

Cloth::Cloth(int width, int height, float spacing)
{
	build_grid(width, height, spacing);
}

Cloth::Cloth(std::vector<glm::vec3> vertices, std::vector<unsigned int> indices)
{
	build_from_mesh(vertices, indices);
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

			particles_.emplace_back(glm::vec3(px, py, 0));
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

	++topology_revision_;
}

void Cloth::build_from_mesh(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices)
{
	if (indices.size() % 3 != 0)
	{
		throw std::invalid_argument("Mesh indices must be triangle-based (multiple of 3).");
	}

	for (unsigned int index : indices)
	{
		if (index >= vertices.size())
		{
			throw std::out_of_range("Mesh index is out of bounds for provided vertices.");
		}
	}

	std::vector<glm::vec3> reordered_vertices;
	std::vector<unsigned int> remapped_indices;
	if (!infer_grid_layout_from_mesh(vertices, indices, width_, height_, spacing_, reordered_vertices, remapped_indices))
	{
		width_ = 0;
		height_ = 0;
		spacing_ = 0.0f;
		reordered_vertices = vertices;
		remapped_indices = indices;
	}

	particles_.clear();
	particles_.reserve(reordered_vertices.size());
	for (const glm::vec3& position : reordered_vertices)
	{
		particles_.emplace_back(position);
	}

	indices_ = std::move(remapped_indices);
	++topology_revision_;
}

void Cloth::set_fixed(int x, int y, bool is_fixed)
{
	if (x < 0 || x >= width_ || y < 0 || y >= height_)
	{
		return;
	}

	particles_[get_index(x, y)].is_fixed = is_fixed;
}

void Cloth::set_fixed(int index, bool is_fixed)
{
	if (index < 0 || index >= static_cast<int>(particles_.size()))
	{
		return;
	}

	particles_[index].is_fixed = is_fixed;
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
