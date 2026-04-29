#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "cloth/core/Particle.h"
#include "cloth/core/Cloth.h"
#include "engine/core/Object.h"

class ClothObject final : public Object
{
public:
	ClothObject(int width, int height, float spacing);
	explicit ClothObject(const std::filesystem::path& mesh_path);

	Cloth& cloth() { return cloth_; }
	const Cloth& cloth() const { return cloth_; }

	void reset_to_initial_state();
	void toggle_anchor_state();
	bool hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const override;

protected:
	bool on_click(const ClickInputEvent& event) override;
	
private:
	void cache_initial_state();
	void refresh_initial_state_if_needed();

	Cloth cloth_;
	std::vector<Particle> initial_particles_;
	std::uint64_t cached_topology_revision_ = 0;
	bool anchors_enabled_ = true;
};
