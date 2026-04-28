#pragma once

#include <glm/glm.hpp>

#include "engine/components/InputComponent.h"

class ClothCameraInputComponent final : public InputComponent
{
public:
	void start() override;
	void update(float delta_time) override;

	bool on_key_pressed(const KeyInputEvent& event) override;
	bool on_key_released(const KeyInputEvent& event) override;
	bool on_pointer_pressed(const PointerInputEvent& event) override;
	bool on_pointer_released(const PointerInputEvent& event) override;
	bool on_pointer_moved(const PointerInputEvent& event) override;
	bool on_wheel_scrolled(const WheelInputEvent& event) override;

private:
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
	float move_speed_ = 1.2f;
	float orbit_sensitivity_ = 0.006f;
	float zoom_step_scale_ = 0.88f;
	float min_distance_ = 0.2f;
	float max_distance_ = 20.0f;
	float max_pitch_ = 1.55334306f;
};
