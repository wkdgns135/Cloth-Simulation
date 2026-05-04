#pragma once

#include <glm/glm.hpp>

#include "engine/components/InputComponent.h"

class ClothCameraInputComponent final : public InputComponent
{
public:
	ClothCameraInputComponent();

	void start() override;
	void update(float delta_time) override;

private:
	bool handle_move_forward_pressed(const KeyInputEvent& event);
	bool handle_move_forward_released(const KeyInputEvent& event);
	bool handle_move_backward_pressed(const KeyInputEvent& event);
	bool handle_move_backward_released(const KeyInputEvent& event);
	bool handle_move_left_pressed(const KeyInputEvent& event);
	bool handle_move_left_released(const KeyInputEvent& event);
	bool handle_move_right_pressed(const KeyInputEvent& event);
	bool handle_move_right_released(const KeyInputEvent& event);
	bool handle_move_up_pressed(const KeyInputEvent& event);
	bool handle_move_up_released(const KeyInputEvent& event);
	bool handle_move_down_pressed(const KeyInputEvent& event);
	bool handle_move_down_released(const KeyInputEvent& event);
	bool handle_right_pointer_pressed(const PointerInputEvent& event);
	bool handle_right_pointer_released(const PointerInputEvent& event);
	bool handle_pointer_moved(const PointerInputEvent& event);
	bool handle_wheel_scrolled(const WheelInputEvent& event);

	void initialize_focus_target();
	void move_camera(float delta_time);
	void orbit_camera(const PointerPosition& delta);
	void zoom_camera(float wheel_steps);

	glm::vec3 focus_target_ = glm::vec3(0.0f);
	bool focus_target_initialized_ = false;
	bool moving_forward_ = false;
	bool moving_backward_ = false;
	bool moving_left_ = false;
	bool moving_right_ = false;
	bool moving_up_ = false;
	bool moving_down_ = false;
	bool rotating_camera_ = false;
	PROPERTY_RANGE(float, move_speed, "Camera Controls", "Move Speed", 1.2f, 0.0f, 100.0f, 0.05f)
	PROPERTY_RANGE(float, orbit_sensitivity, "Camera Controls", "Orbit Sensitivity", 0.006f, 0.0001f, 0.1f, 0.0001f)
	PROPERTY_RANGE(float, zoom_step_scale, "Camera Controls", "Zoom Step Scale", 0.88f, 0.01f, 10.0f, 0.01f)
	PROPERTY_RANGE(float, min_distance, "Camera Controls", "Min Distance", 0.2f, 0.01f, 100.0f, 0.01f)
	PROPERTY_RANGE(float, max_distance, "Camera Controls", "Max Distance", 20.0f, 0.1f, 1000.0f, 0.1f)
	PROPERTY_RANGE(float, max_pitch, "Camera Controls", "Max Pitch", 1.55334306f, 0.1f, 1.56979633f, 0.001f)
};
