#include "engine/core/World.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/objects/CameraObject.h"
#include "engine/objects/DirectionalLightObject.h"

namespace
{
constexpr float kPickRayMinDistance = 0.000001f;

template <typename ListenerContainer, typename Dispatcher>
bool dispatch_input_listeners(const ListenerContainer& listeners, Dispatcher&& dispatcher)
{
	for (auto it = listeners.rbegin(); it != listeners.rend(); ++it)
	{
		if (it->handler && dispatcher(*it))
		{
			return true;
		}
	}

	return false;
}

template <typename ListenerContainer, typename Dispatcher>
bool broadcast_input_listeners(const ListenerContainer& listeners, Dispatcher&& dispatcher)
{
	bool handled = false;

	for (auto it = listeners.rbegin(); it != listeners.rend(); ++it)
	{
		if (it->handler && dispatcher(*it))
		{
			handled = true;
		}
	}

	return handled;
}

template <typename ListenerContainer>
void remove_input_listener(ListenerContainer& listeners, InputSubscriptionHandle handle)
{
	listeners.erase(
		std::remove_if(listeners.begin(), listeners.end(), [&](const auto& listener) {
			return listener.handle.id == handle.id;
		}),
		listeners.end());
}

template <typename ListenerContainer, typename Handler>
InputSubscriptionHandle add_input_listener(
	ListenerContainer& listeners,
	std::uint64_t& next_input_subscription_id,
	Handler&& handler)
{
	if (!handler)
	{
		return {};
	}

	InputSubscriptionHandle handle{ next_input_subscription_id++ };
	listeners.push_back(typename ListenerContainer::value_type{ handle, std::forward<Handler>(handler) });
	return handle;
}

glm::vec3 project_point_from_clip_space(const glm::mat4& inverse_view_projection, const glm::vec3& clip_point)
{
	const glm::vec4 world = inverse_view_projection * glm::vec4(clip_point, 1.0f);
	if (std::abs(world.w) <= kPickRayMinDistance)
	{
		return glm::vec3(world);
	}

	return glm::vec3(world) / world.w;
}
}

World::World()
	: main_camera_object_(&create_object<CameraObject>())
	, main_directional_light_object_(&create_object<DirectionalLightObject>())
{
	main_camera_object_->transform().position = glm::vec3(0.0f, 0.0f, 2.0f);
	main_camera_object_->transform().look_at(glm::vec3(0.0f));

	main_directional_light_object_->transform().set_forward(glm::normalize(glm::vec3(-0.35f, 0.65f, 0.70f)));
}

void World::awake()
{
	if (awakened_)
	{
		return;
	}

	awakened_ = true;
	destroyed_ = false;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->awake();
	}
}

void World::start()
{
	if (started_)
	{
		return;
	}

	awake();
	started_ = true;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->start();
	}
}

void World::update(float delta_time)
{
	for (std::size_t i = 0; i < objects_.size(); ++i)
	{
		objects_[i]->update(delta_time);
	}
}

void World::stop()
{
	if (!started_)
	{
		return;
	}

	started_ = false;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->stop();
	}
}

void World::destroy()
{
	if (destroyed_)
	{
		return;
	}

	stop();

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->destroy();
	}

	for (auto& listeners : key_pressed_listeners_)
	{
		listeners.clear();
	}
	for (auto& listeners : key_released_listeners_)
	{
		listeners.clear();
	}
	for (auto& listeners : pointer_pressed_listeners_)
	{
		listeners.clear();
	}
	for (auto& listeners : pointer_released_listeners_)
	{
		listeners.clear();
	}
	pointer_moved_listeners_.clear();
	wheel_scrolled_listeners_.clear();
	hovered_object_ = nullptr;

	destroyed_ = true;
	awakened_ = false;
}

bool World::on_key_pressed(const KeyInputEvent& event)
{
	bool handled = native_on_key_pressed(event);

	handled = broadcast_input_listeners(
		key_pressed_listeners_[static_cast<std::size_t>(event.key)],
		[&](const auto& listener) {
			return listener.handler(event);
		}) || handled;

	return handled;
}

bool World::on_key_released(const KeyInputEvent& event)
{
	bool handled = native_on_key_released(event);

	handled = broadcast_input_listeners(
		key_released_listeners_[static_cast<std::size_t>(event.key)],
		[&](const auto& listener) {
			return listener.handler(event);
		}) || handled;

	return handled;
}

bool World::on_pointer_pressed(const PointerInputEvent& event)
{
	bool handled = native_on_pointer_pressed(event);

	handled = dispatch_input_listeners(
		pointer_pressed_listeners_[static_cast<std::size_t>(event.button)],
		[&](const auto& listener) {
			return listener.handler(event);
		}) || handled;

	return handled;
}

bool World::on_pointer_released(const PointerInputEvent& event)
{
	bool handled = native_on_pointer_released(event);

	handled = dispatch_input_listeners(
		pointer_released_listeners_[static_cast<std::size_t>(event.button)],
		[&](const auto& listener) {
			return listener.handler(event);
		}) || handled;

	return handled;
}

bool World::on_pointer_moved(const PointerInputEvent& event)
{
	bool handled = native_on_pointer_moved(event);

	handled = dispatch_input_listeners(pointer_moved_listeners_, [&](const auto& listener) {
		return listener.handler(event);
	}) || handled;

	update_hovered_object(event.position);
	return handled;
}

bool World::on_click(const ClickInputEvent& event)
{
	bool handled = native_on_click(event);

	if (Object* clicked_object = pick_object(event.position))
	{
		handled = clicked_object->on_click(event) || handled;
	}

	return handled;
}

bool World::on_wheel_scrolled(const WheelInputEvent& event)
{
	bool handled = native_on_wheel_scrolled(event);

	handled = dispatch_input_listeners(wheel_scrolled_listeners_, [&](const auto& listener) {
		return listener.handler(event);
	}) || handled;

	return handled;
}

void World::set_viewport_size(int width, int height)
{
	viewport_width_ = width > 0 ? width : 1;
	viewport_height_ = height > 0 ? height : 1;
}

RenderScene World::build_render_scene() const
{
	RenderScene scene;

	if (main_camera_object_)
	{
		scene.camera = main_camera_object_->build_camera();
	}

	if (main_directional_light_object_)
	{
		scene.directional_light = main_directional_light_object_->build_light();
	}

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->collect_render_data(scene);
	}

	return scene;
}

InputSubscriptionHandle World::subscribe_key_pressed(InputKey key, InputComponent::KeyHandler handler)
{
	if (key == InputKey::Unknown)
	{
		return {};
	}

	return add_input_listener(
		key_pressed_listeners_[static_cast<std::size_t>(key)],
		next_input_subscription_id_,
		std::move(handler));
}

InputSubscriptionHandle World::subscribe_key_released(InputKey key, InputComponent::KeyHandler handler)
{
	if (key == InputKey::Unknown)
	{
		return {};
	}

	return add_input_listener(
		key_released_listeners_[static_cast<std::size_t>(key)],
		next_input_subscription_id_,
		std::move(handler));
}

InputSubscriptionHandle World::subscribe_pointer_pressed(PointerButton button, InputComponent::PointerHandler handler)
{
	if (button == PointerButton::None)
	{
		return {};
	}

	return add_input_listener(
		pointer_pressed_listeners_[static_cast<std::size_t>(button)],
		next_input_subscription_id_,
		std::move(handler));
}

InputSubscriptionHandle World::subscribe_pointer_released(PointerButton button, InputComponent::PointerHandler handler)
{
	if (button == PointerButton::None)
	{
		return {};
	}

	return add_input_listener(
		pointer_released_listeners_[static_cast<std::size_t>(button)],
		next_input_subscription_id_,
		std::move(handler));
}

InputSubscriptionHandle World::subscribe_pointer_moved(InputComponent::PointerHandler handler)
{
	return add_input_listener(pointer_moved_listeners_, next_input_subscription_id_, std::move(handler));
}

InputSubscriptionHandle World::subscribe_wheel_scrolled(InputComponent::WheelHandler handler)
{
	return add_input_listener(wheel_scrolled_listeners_, next_input_subscription_id_, std::move(handler));
}

void World::unsubscribe_key_pressed(InputKey key, InputSubscriptionHandle handle)
{
	if (key == InputKey::Unknown || !handle.is_valid())
	{
		return;
	}

	remove_input_listener(key_pressed_listeners_[static_cast<std::size_t>(key)], handle);
}

void World::unsubscribe_key_released(InputKey key, InputSubscriptionHandle handle)
{
	if (key == InputKey::Unknown || !handle.is_valid())
	{
		return;
	}

	remove_input_listener(key_released_listeners_[static_cast<std::size_t>(key)], handle);
}

void World::unsubscribe_pointer_pressed(PointerButton button, InputSubscriptionHandle handle)
{
	if (button == PointerButton::None || !handle.is_valid())
	{
		return;
	}

	remove_input_listener(pointer_pressed_listeners_[static_cast<std::size_t>(button)], handle);
}

void World::unsubscribe_pointer_released(PointerButton button, InputSubscriptionHandle handle)
{
	if (button == PointerButton::None || !handle.is_valid())
	{
		return;
	}

	remove_input_listener(pointer_released_listeners_[static_cast<std::size_t>(button)], handle);
}

void World::unsubscribe_pointer_moved(InputSubscriptionHandle handle)
{
	if (!handle.is_valid())
	{
		return;
	}

	remove_input_listener(pointer_moved_listeners_, handle);
}

void World::unsubscribe_wheel_scrolled(InputSubscriptionHandle handle)
{
	if (!handle.is_valid())
	{
		return;
	}

	remove_input_listener(wheel_scrolled_listeners_, handle);
}

Object* World::pick_object(const PointerPosition& position) const
{
	if (!main_camera_object_)
	{
		return nullptr;
	}

	const Camera camera = main_camera_object_->build_camera();
	const float aspect = static_cast<float>(viewport_width_) / static_cast<float>(viewport_height_);
	const glm::mat4 view = glm::lookAt(camera.position, camera.target, camera.up);
	const glm::mat4 projection = glm::perspective(
		glm::radians(camera.fov_y_degrees),
		aspect,
		camera.near_plane,
		camera.far_plane);
	const glm::mat4 inverse_view_projection = glm::inverse(projection * view);

	const float ndc_x = (2.0f * position.x) / static_cast<float>(viewport_width_) - 1.0f;
	const float ndc_y = 1.0f - (2.0f * position.y) / static_cast<float>(viewport_height_);

	const glm::vec3 near_point = project_point_from_clip_space(inverse_view_projection, glm::vec3(ndc_x, ndc_y, -1.0f));
	const glm::vec3 far_point = project_point_from_clip_space(inverse_view_projection, glm::vec3(ndc_x, ndc_y, 1.0f));
	const glm::vec3 ray_direction = far_point - near_point;
	if (glm::dot(ray_direction, ray_direction) <= kPickRayMinDistance)
	{
		return nullptr;
	}

	const glm::vec3 normalized_ray_direction = glm::normalize(ray_direction);

	Object* closest_object = nullptr;
	float closest_hit_distance = 0.0f;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		float hit_distance = 0.0f;
		if (!object->hit_test(camera.position, normalized_ray_direction, hit_distance))
		{
			continue;
		}

		if (!closest_object || hit_distance < closest_hit_distance)
		{
			closest_object = object.get();
			closest_hit_distance = hit_distance;
		}
	}

	return closest_object;
}

void World::update_hovered_object(const PointerPosition& position)
{
	Object* next_hovered_object = pick_object(position);
	if (next_hovered_object == hovered_object_)
	{
		return;
	}

	if (hovered_object_)
	{
		hovered_object_->on_hover_leave(position);
	}

	hovered_object_ = next_hovered_object;

	if (hovered_object_)
	{
		hovered_object_->on_hover_enter(position);
	}
}

bool World::native_on_key_pressed(const KeyInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

bool World::native_on_key_released(const KeyInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

bool World::native_on_pointer_pressed(const PointerInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

bool World::native_on_pointer_released(const PointerInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

bool World::native_on_pointer_moved(const PointerInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

bool World::native_on_click(const ClickInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

bool World::native_on_wheel_scrolled(const WheelInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

bool World::destroy_object(Object* object)
{
	if (!object)
	{
		return false;
	}

	const auto it = std::find_if(objects_.begin(), objects_.end(), [&](const std::unique_ptr<Object>& candidate) {
		return candidate.get() == object;
	});
	if (it == objects_.end())
	{
		return false;
	}

	if (hovered_object_ == object)
	{
		hovered_object_ = nullptr;
	}

	(*it)->destroy();
	objects_.erase(it);
	return true;
}
