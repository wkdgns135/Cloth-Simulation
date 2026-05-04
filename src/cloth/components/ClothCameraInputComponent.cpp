#include "cloth/components/ClothCameraInputComponent.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

#include "engine/components/TransformComponent.h"
#include "engine/core/WorldObject.h"

namespace
{
constexpr float kVectorLengthThreshold = 0.000001f;

glm::vec3 safe_normalize(const glm::vec3& value, const glm::vec3& fallback)
{
	if (glm::dot(value, value) <= kVectorLengthThreshold)
	{
		return fallback;
	}

	return glm::normalize(value);
}
}

ClothCameraInputComponent::ClothCameraInputComponent()
{
	set_display_name("Camera Controls");

	bind_key_pressed(InputKey::W, this, &ClothCameraInputComponent::handle_move_forward_pressed);
	bind_key_pressed(InputKey::Up, this, &ClothCameraInputComponent::handle_move_forward_pressed);
	bind_key_pressed(InputKey::S, this, &ClothCameraInputComponent::handle_move_backward_pressed);
	bind_key_pressed(InputKey::Down, this, &ClothCameraInputComponent::handle_move_backward_pressed);
	bind_key_pressed(InputKey::A, this, &ClothCameraInputComponent::handle_move_left_pressed);
	bind_key_pressed(InputKey::Left, this, &ClothCameraInputComponent::handle_move_left_pressed);
	bind_key_pressed(InputKey::D, this, &ClothCameraInputComponent::handle_move_right_pressed);
	bind_key_pressed(InputKey::Right, this, &ClothCameraInputComponent::handle_move_right_pressed);
	bind_key_pressed(InputKey::E, this, &ClothCameraInputComponent::handle_move_up_pressed);
	bind_key_pressed(InputKey::Q, this, &ClothCameraInputComponent::handle_move_down_pressed);

	bind_key_released(InputKey::W, this, &ClothCameraInputComponent::handle_move_forward_released);
	bind_key_released(InputKey::Up, this, &ClothCameraInputComponent::handle_move_forward_released);
	bind_key_released(InputKey::S, this, &ClothCameraInputComponent::handle_move_backward_released);
	bind_key_released(InputKey::Down, this, &ClothCameraInputComponent::handle_move_backward_released);
	bind_key_released(InputKey::A, this, &ClothCameraInputComponent::handle_move_left_released);
	bind_key_released(InputKey::Left, this, &ClothCameraInputComponent::handle_move_left_released);
	bind_key_released(InputKey::D, this, &ClothCameraInputComponent::handle_move_right_released);
	bind_key_released(InputKey::Right, this, &ClothCameraInputComponent::handle_move_right_released);
	bind_key_released(InputKey::E, this, &ClothCameraInputComponent::handle_move_up_released);
	bind_key_released(InputKey::Q, this, &ClothCameraInputComponent::handle_move_down_released);

	bind_pointer_pressed(PointerButton::Right, this, &ClothCameraInputComponent::handle_right_pointer_pressed);
	bind_pointer_released(PointerButton::Right, this, &ClothCameraInputComponent::handle_right_pointer_released);
	bind_pointer_moved(this, &ClothCameraInputComponent::handle_pointer_moved);
	bind_wheel_scrolled(this, &ClothCameraInputComponent::handle_wheel_scrolled);
}

void ClothCameraInputComponent::start()
{
	initialize_focus_target();
}

void ClothCameraInputComponent::update(float delta_time)
{
	if (delta_time <= 0.0f)
	{
		return;
	}

	if (!moving_forward_
		&& !moving_backward_
		&& !moving_left_
		&& !moving_right_
		&& !moving_up_
		&& !moving_down_)
	{
		return;
	}

	move_camera(delta_time);
}

bool ClothCameraInputComponent::handle_move_forward_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_forward_ = true;
	return true;
}

bool ClothCameraInputComponent::handle_move_forward_released(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_forward_ = false;
	return true;
}

bool ClothCameraInputComponent::handle_move_backward_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_backward_ = true;
	return true;
}

bool ClothCameraInputComponent::handle_move_backward_released(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_backward_ = false;
	return true;
}

bool ClothCameraInputComponent::handle_move_left_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_left_ = true;
	return true;
}

bool ClothCameraInputComponent::handle_move_left_released(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_left_ = false;
	return true;
}

bool ClothCameraInputComponent::handle_move_right_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_right_ = true;
	return true;
}

bool ClothCameraInputComponent::handle_move_right_released(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_right_ = false;
	return true;
}

bool ClothCameraInputComponent::handle_move_up_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_up_ = true;
	return true;
}

bool ClothCameraInputComponent::handle_move_up_released(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_up_ = false;
	return true;
}

bool ClothCameraInputComponent::handle_move_down_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_down_ = true;
	return true;
}

bool ClothCameraInputComponent::handle_move_down_released(const KeyInputEvent& event)
{
	static_cast<void>(event);
	moving_down_ = false;
	return true;
}

bool ClothCameraInputComponent::handle_right_pointer_pressed(const PointerInputEvent& event)
{
	static_cast<void>(event);
	rotating_camera_ = true;
	initialize_focus_target();
	return true;
}

bool ClothCameraInputComponent::handle_right_pointer_released(const PointerInputEvent& event)
{
	static_cast<void>(event);
	rotating_camera_ = false;
	return true;
}

bool ClothCameraInputComponent::handle_pointer_moved(const PointerInputEvent& event)
{
	if (!rotating_camera_)
	{
		return false;
	}

	orbit_camera(event.delta);
	return true;
}

bool ClothCameraInputComponent::handle_wheel_scrolled(const WheelInputEvent& event)
{
	if (event.steps == 0.0f)
	{
		return false;
	}

	zoom_camera(event.steps);
	return true;
}

void ClothCameraInputComponent::initialize_focus_target()
{
	if (focus_target_initialized_)
	{
		return;
	}

	WorldObject* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	const TransformComponent& transform = object_owner->transform();
	const glm::vec3 position = transform.position();
	const glm::vec3 forward = safe_normalize(transform.forward(), glm::vec3(0.0f, 0.0f, -1.0f));
	const float default_distance = std::max(glm::length(position), 1.0f);
	focus_target_ = position + forward * default_distance;
	focus_target_initialized_ = true;
}

void ClothCameraInputComponent::move_camera(float delta_time)
{
	WorldObject* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	initialize_focus_target();

	TransformComponent& transform = object_owner->transform();
	const glm::vec3 forward = safe_normalize(transform.forward(), glm::vec3(0.0f, 0.0f, -1.0f));
	const glm::vec3 up = safe_normalize(transform.up(), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::vec3 right = safe_normalize(transform.right(), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::vec3 movement(0.0f);
	if (moving_forward_)
	{
		movement += forward;
	}
	if (moving_backward_)
	{
		movement -= forward;
	}
	if (moving_right_)
	{
		movement += right;
	}
	if (moving_left_)
	{
		movement -= right;
	}
	if (moving_up_)
	{
		movement += up;
	}
	if (moving_down_)
	{
		movement -= up;
	}

	if (glm::dot(movement, movement) <= kVectorLengthThreshold)
	{
		return;
	}

	const glm::vec3 offset = glm::normalize(movement) * move_speed() * delta_time;
	transform.set_position(transform.position() + offset);
	focus_target_ += offset;
}

void ClothCameraInputComponent::orbit_camera(const PointerPosition& delta)
{
	WorldObject* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	initialize_focus_target();

	TransformComponent& transform = object_owner->transform();
	const glm::vec3 offset = transform.position() - focus_target_;
	const float radius = glm::length(offset);
	if (radius <= min_distance())
	{
		return;
	}

	float yaw = std::atan2(offset.x, offset.z);
	float pitch = std::asin(std::clamp(offset.y / radius, -1.0f, 1.0f));

	yaw -= delta.x * orbit_sensitivity();
	pitch += delta.y * orbit_sensitivity();
	pitch = std::clamp(pitch, -max_pitch(), max_pitch());

	const float cos_pitch = std::cos(pitch);
	transform.set_position(focus_target_ + glm::vec3(
		std::sin(yaw) * cos_pitch * radius,
		std::sin(pitch) * radius,
		std::cos(yaw) * cos_pitch * radius));
	transform.look_at(focus_target_);
}

void ClothCameraInputComponent::zoom_camera(float wheel_steps)
{
	WorldObject* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	initialize_focus_target();

	TransformComponent& transform = object_owner->transform();
	glm::vec3 offset = transform.position() - focus_target_;
	float distance = glm::length(offset);

	if (distance <= kVectorLengthThreshold)
	{
		offset = glm::vec3(0.0f, 0.0f, 1.0f);
		distance = 1.0f;
	}

	const glm::vec3 direction = offset / distance;
	const float next_distance = std::clamp(
		distance * std::pow(zoom_step_scale(), wheel_steps),
		min_distance(),
		max_distance());

	transform.set_position(focus_target_ + direction * next_distance);
	transform.look_at(focus_target_);
}
