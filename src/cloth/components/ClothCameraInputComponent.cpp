#include "cloth/components/ClothCameraInputComponent.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

#include "engine/core/Object.h"
#include "engine/core/Transform.h"

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

bool ClothCameraInputComponent::on_key_pressed(const KeyInputEvent& event)
{
	switch (event.key)
	{
	case InputKey::W:
	case InputKey::Up:
		moving_forward_ = true;
		return true;
	case InputKey::S:
	case InputKey::Down:
		moving_backward_ = true;
		return true;
	case InputKey::A:
	case InputKey::Left:
		moving_left_ = true;
		return true;
	case InputKey::D:
	case InputKey::Right:
		moving_right_ = true;
		return true;
	case InputKey::E:
		moving_up_ = true;
		return true;
	case InputKey::Q:
		moving_down_ = true;
		return true;
	default:
		return false;
	}
}

bool ClothCameraInputComponent::on_key_released(const KeyInputEvent& event)
{
	switch (event.key)
	{
	case InputKey::W:
	case InputKey::Up:
		moving_forward_ = false;
		return true;
	case InputKey::S:
	case InputKey::Down:
		moving_backward_ = false;
		return true;
	case InputKey::A:
	case InputKey::Left:
		moving_left_ = false;
		return true;
	case InputKey::D:
	case InputKey::Right:
		moving_right_ = false;
		return true;
	case InputKey::E:
		moving_up_ = false;
		return true;
	case InputKey::Q:
		moving_down_ = false;
		return true;
	default:
		return false;
	}
}

bool ClothCameraInputComponent::on_pointer_pressed(const PointerInputEvent& event)
{
	if (event.button != PointerButton::Left)
	{
		return false;
	}

	rotating_camera_ = true;
	initialize_focus_target();
	return true;
}

bool ClothCameraInputComponent::on_pointer_released(const PointerInputEvent& event)
{
	if (event.button != PointerButton::Left)
	{
		return false;
	}

	rotating_camera_ = false;
	return true;
}

bool ClothCameraInputComponent::on_pointer_moved(const PointerInputEvent& event)
{
	if (!rotating_camera_)
	{
		return false;
	}

	orbit_camera(event.delta);
	return true;
}

bool ClothCameraInputComponent::on_wheel_scrolled(const WheelInputEvent& event)
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

	Object* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	const Transform& transform = object_owner->transform();
	const glm::vec3 forward = safe_normalize(transform.forward(), glm::vec3(0.0f, 0.0f, -1.0f));
	const float default_distance = std::max(glm::length(transform.position), 1.0f);
	focus_target_ = transform.position + forward * default_distance;
	focus_target_initialized_ = true;
}

void ClothCameraInputComponent::move_camera(float delta_time)
{
	Object* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	initialize_focus_target();

	Transform& transform = object_owner->transform();
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

	const glm::vec3 offset = glm::normalize(movement) * move_speed_ * delta_time;
	transform.position += offset;
	focus_target_ += offset;
}

void ClothCameraInputComponent::orbit_camera(const PointerPosition& delta)
{
	Object* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	initialize_focus_target();

	Transform& transform = object_owner->transform();
	const glm::vec3 offset = transform.position - focus_target_;
	const float radius = glm::length(offset);
	if (radius <= min_distance_)
	{
		return;
	}

	float yaw = std::atan2(offset.x, offset.z);
	float pitch = std::asin(std::clamp(offset.y / radius, -1.0f, 1.0f));

	yaw -= delta.x * orbit_sensitivity_;
	pitch += delta.y * orbit_sensitivity_;
	pitch = std::clamp(pitch, -max_pitch_, max_pitch_);

	const float cos_pitch = std::cos(pitch);
	transform.position = focus_target_ + glm::vec3(
		std::sin(yaw) * cos_pitch * radius,
		std::sin(pitch) * radius,
		std::cos(yaw) * cos_pitch * radius);
	transform.look_at(focus_target_);
}

void ClothCameraInputComponent::zoom_camera(float wheel_steps)
{
	Object* object_owner = owner();
	if (!object_owner)
	{
		return;
	}

	initialize_focus_target();

	Transform& transform = object_owner->transform();
	glm::vec3 offset = transform.position - focus_target_;
	float distance = glm::length(offset);

	if (distance <= kVectorLengthThreshold)
	{
		offset = glm::vec3(0.0f, 0.0f, 1.0f);
		distance = 1.0f;
	}

	const glm::vec3 direction = offset / distance;
	const float next_distance = std::clamp(
		distance * std::pow(zoom_step_scale_, wheel_steps),
		min_distance_,
		max_distance_);

	transform.position = focus_target_ + direction * next_distance;
	transform.look_at(focus_target_);
}
