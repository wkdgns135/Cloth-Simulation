#include "cloth/components/ClothInteractionComponent.h"

#include <algorithm>
#include <cstddef>

#include "cloth/core/Cloth.h"

ClothInteractionComponent::ClothInteractionComponent(Cloth& cloth)
	: cloth_(cloth)
{
	cache_initial_state();
}

bool ClothInteractionComponent::on_key_pressed(const KeyInputEvent& event)
{
	switch (event.key)
	{
	case InputKey::R:
		restore_initial_state();
		return true;
	case InputKey::Space:
		toggle_anchor_state();
		return true;
	default:
		return false;
	}
}

void ClothInteractionComponent::cache_initial_state()
{
	initial_particles_ = cloth_.get_particles();
	cached_topology_revision_ = cloth_.get_topology_revision();
	anchors_enabled_ = true;
}

void ClothInteractionComponent::refresh_initial_state_if_needed()
{
	if (cached_topology_revision_ != cloth_.get_topology_revision()
		|| initial_particles_.size() != cloth_.get_particles().size())
	{
		cache_initial_state();
	}
}

void ClothInteractionComponent::restore_initial_state()
{
	refresh_initial_state_if_needed();
	cloth_.get_particles() = initial_particles_;
	anchors_enabled_ = true;
}

void ClothInteractionComponent::toggle_anchor_state()
{
	refresh_initial_state_if_needed();
	anchors_enabled_ = !anchors_enabled_;

	auto& particles = cloth_.get_particles();
	const std::size_t particle_count = std::min(particles.size(), initial_particles_.size());

	for (std::size_t i = 0; i < particle_count; ++i)
	{
		particles[i].is_fixed = anchors_enabled_ ? initial_particles_[i].is_fixed : false;

		if (particles[i].is_fixed)
		{
			particles[i].prev_position = particles[i].position;
		}
	}
}
