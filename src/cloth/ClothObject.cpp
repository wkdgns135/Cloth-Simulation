#include "cloth/ClothObject.h"

#include <algorithm>
#include <vector>

#include "cloth/components/ClothRenderComponent.h"
#include "io/MeshLoader.h"

namespace
{
void pin_grid_top_row(Cloth& cloth)
{
	for (int i = 0; i < cloth.get_width(); ++i)
	{
		cloth.set_fixed(i, 0, true);
	}
}

void pin_highest_particles(Cloth& cloth, int count)
{
	auto& particles = cloth.get_particles();
	if (particles.empty() || count <= 0)
	{
		return;
	}

	std::vector<int> sorted_indices;
	sorted_indices.reserve(particles.size());
	for (int i = 0; i < static_cast<int>(particles.size()); ++i)
	{
		sorted_indices.push_back(i);
	}

	std::sort(sorted_indices.begin(), sorted_indices.end(), [&](int lhs, int rhs)
		{
			return particles[lhs].position.y > particles[rhs].position.y;
		});

	const int pinned_count = std::min(count, static_cast<int>(sorted_indices.size()));
	for (int i = 0; i < pinned_count; ++i)
	{
		cloth.set_fixed(sorted_indices[i], true);
	}
}
}

ClothObject::ClothObject(int width, int height, float spacing)
	: cloth_(width, height, spacing)
{
	pin_grid_top_row(cloth_);
	add_component<ClothRenderComponent>(cloth_);
}

ClothObject::ClothObject(const std::filesystem::path& mesh_path)
	: cloth_(io::load_cloth(mesh_path))
{
	pin_highest_particles(cloth_, 2);
	add_component<ClothRenderComponent>(cloth_);
}
