#include "cloth/components/ClothInteractionComponent.h"

#include "cloth/ClothObject.h"

ClothInteractionComponent::ClothInteractionComponent(ClothObject& cloth_object)
	: cloth_object_(cloth_object)
{
	bind_key_pressed(InputKey::R, this, &ClothInteractionComponent::handle_reset_pressed);
	bind_key_pressed(InputKey::Space, this, &ClothInteractionComponent::handle_toggle_anchor_pressed);
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
