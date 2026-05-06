#pragma once

#include "engine/components/InputComponent.h"

class ClothObject;

class ClothInteractionComponent final : public InputComponent
{
public:
	explicit ClothInteractionComponent(ClothObject& cloth_object);

private:
	bool handle_reset_pressed(const KeyInputEvent& event);
	bool handle_toggle_anchor_pressed(const KeyInputEvent& event);
	bool handle_left_pointer_pressed(const PointerInputEvent& event);
	bool handle_left_pointer_released(const PointerInputEvent& event);
	bool handle_pointer_moved(const PointerInputEvent& event);

	ClothObject& cloth_object_;
};
