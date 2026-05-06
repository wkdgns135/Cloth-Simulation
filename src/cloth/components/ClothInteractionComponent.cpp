#include "cloth/components/ClothInteractionComponent.h"

#include "cloth/ClothObject.h"

ClothInteractionComponent::ClothInteractionComponent(ClothObject& cloth_object)
	: cloth_object_(cloth_object)
{
	bind_key_pressed(InputKey::R, this, &ClothInteractionComponent::handle_reset_pressed);
	bind_key_pressed(InputKey::Space, this, &ClothInteractionComponent::handle_toggle_anchor_pressed);
	bind_pointer_pressed(PointerButton::Left, this, &ClothInteractionComponent::handle_left_pointer_pressed);
	bind_pointer_released(PointerButton::Left, this, &ClothInteractionComponent::handle_left_pointer_released);
	bind_pointer_moved(this, &ClothInteractionComponent::handle_pointer_moved);
}

bool ClothInteractionComponent::handle_reset_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	cloth_object_.reset_to_initial_state();
	return true;
}

bool ClothInteractionComponent::handle_toggle_anchor_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	cloth_object_.toggle_anchor_state();
	return true;
}

bool ClothInteractionComponent::handle_left_pointer_pressed(const PointerInputEvent& event)
{
	return cloth_object_.begin_particle_grab(event.position);
}

bool ClothInteractionComponent::handle_left_pointer_released(const PointerInputEvent& event)
{
	static_cast<void>(event);
	return cloth_object_.end_particle_grab();
}

bool ClothInteractionComponent::handle_pointer_moved(const PointerInputEvent& event)
{
	return cloth_object_.update_particle_grab(event.position);
}
