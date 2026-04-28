#pragma once

#include <cstdint>
#include <vector>

#include "cloth/core/Particle.h"
#include "engine/components/InputComponent.h"

class Cloth;

class ClothInteractionComponent final : public InputComponent
{
public:
	explicit ClothInteractionComponent(Cloth& cloth);

	bool on_key_pressed(const KeyInputEvent& event) override;

private:
	void cache_initial_state();
	void refresh_initial_state_if_needed();
	void restore_initial_state();
	void toggle_anchor_state();

	Cloth& cloth_;
	std::vector<Particle> initial_particles_;
	std::uint64_t cached_topology_revision_ = 0;
	bool anchors_enabled_ = true;
};
