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
{
	main_camera_object_ = &create_object<CameraObject>();
	main_directional_light_object_ = &create_object<DirectionalLightObject>();

	main_camera_object_->set_object_world_position(glm::vec3(0.0f, 0.0f, 2.0f));
	main_camera_object_->look_at_world_position(glm::vec3(0.0f));

	main_directional_light_object_->set_object_forward_direction(glm::normalize(glm::vec3(-0.35f, 0.65f, 0.70f)));
}

void World::awake()
{
	if (awakened_)
	{
		return;
	}

	awakened_ = true;
	destroyed_ = false;

	for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
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

	for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
	{
		object->start();
	}
}

void World::update(float delta_time)
{
	updating_ = true;

	std::vector<WorldObject*> update_objects;
	update_objects.reserve(world_objects_.size());

	for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
	{
		if (!object)
		{
			continue;
		}

		WorldObject* raw_object = object.get();
		if (is_destroy_queued(raw_object) || raw_object->destroy_requested())
		{
			continue;
		}

		update_objects.push_back(raw_object);
	}

	std::stable_sort(update_objects.begin(), update_objects.end(), [](const WorldObject* lhs, const WorldObject* rhs) {
		return lhs->update_order() < rhs->update_order();
	});

	for (WorldObject* object : update_objects)
	{
		if (is_destroy_queued(object) || object->destroy_requested())
		{
			continue;
		}

		object->update(delta_time);
	}

	updating_ = false;
	flush_destroy_requests();
}

void World::stop()
{
	if (!started_)
	{
		return;
	}

	started_ = false;

	for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
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

	for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
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

	if (WorldObject* clicked_object = pick_object(event.position))
	{
		handled = on_world_object_clicked(*clicked_object, event) || handled;
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

Object* World::find_runtime_object(ObjectId object_id)
{
	if (WorldObject* world_object = find_object(object_id))
	{
		return world_object;
	}

	for (const std::unique_ptr<WorldObject>& world_object : world_objects_.ordered())
	{
		if (!world_object)
		{
			continue;
		}

		if (Component* component = world_object->find_component<Component>(object_id))
		{
			return component;
		}
	}

	return nullptr;
}

const Object* World::find_runtime_object(ObjectId object_id) const
{
	if (const WorldObject* world_object = find_object(object_id))
	{
		return world_object;
	}

	for (const std::unique_ptr<WorldObject>& world_object : world_objects_.ordered())
	{
		if (!world_object)
		{
			continue;
		}

		if (const Component* component = world_object->find_component<Component>(object_id))
		{
			return component;
		}
	}

	return nullptr;
}

bool World::set_runtime_object_property(ObjectId object_id, std::string_view property_id, const PropertyValue& value)
{
	if (Object* object = find_runtime_object(object_id))
	{
		return object->set_property(property_id, value);
	}

	return false;
}

bool World::select_object(ObjectId object_id)
{
	if (object_id == 0)
	{
		if (selected_object_id_ == 0)
		{
			return true;
		}

		selected_object_id_ = 0;
		notify_selection_changed();
		return true;
	}

	if (!find_object(object_id))
	{
		return false;
	}

	if (selected_object_id_ == object_id)
	{
		return true;
	}

	selected_object_id_ = object_id;
	notify_selection_changed();
	return true;
}

void World::set_change_callback(ChangeCallback change_callback)
{
	change_callback_ = std::move(change_callback);
}

void World::notify_world_object_property_changed(const WorldObject& object, const PropertyBase& property)
{
	on_world_object_property_changed(object, property);
}

void World::notify_component_property_changed(const WorldObject& owner, const Component& component, const PropertyBase& property)
{
	on_component_property_changed(owner, component, property);
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

	for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
	{
		object->collect_render_data(scene);
	}

	return scene;
}

void World::notify_snapshot_invalidated() const
{
	if (change_callback_)
	{
		change_callback_(ChangeEvent{ ChangeEvent::Kind::SnapshotInvalidated });
	}
}

void World::notify_selection_changed() const
{
	if (change_callback_)
	{
		ChangeEvent event;
		event.kind = ChangeEvent::Kind::SelectionChanged;
		event.selected_object_id = selected_object_id_;
		change_callback_(event);
	}
}

void World::notify_object_value_changed(
	ObjectId object_id,
	ObjectId source_object_id,
	std::string value_id,
	PropertyValue value) const
{
	if (change_callback_)
	{
		ChangeEvent event;
		event.kind = ChangeEvent::Kind::ObjectValueChanged;
		event.object_id = object_id;
		event.selected_object_id = selected_object_id_;
		event.source_object_id = source_object_id;
		event.value_id = std::move(value_id);
		event.value = std::move(value);
		change_callback_(event);
	}
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

WorldObject* World::pick_object(const PointerPosition& position) const
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

	WorldObject* closest_object = nullptr;
	float closest_hit_distance = 0.0f;

	for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
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
	WorldObject* next_hovered_object = pick_object(position);
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

bool World::on_world_object_clicked(WorldObject& object, const ClickInputEvent& event)
{
	static_cast<void>(event);
	return select_object(object.id());
}

void World::on_world_object_property_changed(const WorldObject& object, const PropertyBase& property)
{
	notify_object_value_changed(object.id(), object.id(), std::string(property.id()), property.value());
}

void World::on_component_property_changed(const WorldObject& owner, const Component& component, const PropertyBase& property)
{
	notify_object_value_changed(owner.id(), component.id(), std::string(property.id()), property.value());
}

bool World::destroy_object(WorldObject* object)
{
	if (!object)
	{
		return false;
	}

	if (updating_)
	{
		request_destroy_object(object);
		return true;
	}

	return destroy_object_immediate(object);
}

void World::request_destroy_object(WorldObject* object)
{
	if (!object)
	{
		return;
	}

	if (is_destroy_queued(object))
	{
		return;
	}

	pending_destroy_objects_.push_back(object);
}

bool World::destroy_object_immediate(WorldObject* object)
{
	if (!object)
	{
		return false;
	}

	WorldObject* existing_object = world_objects_.find(object->id());
	if (!existing_object)
	{
		return false;
	}

	if (hovered_object_ == object)
	{
		hovered_object_ = nullptr;
	}

	const bool was_selected = selected_object_id_ == object->id();
	if (was_selected)
	{
		selected_object_id_ = 0;
	}

	existing_object->destroy();
	const bool erased = world_objects_.erase(existing_object);
	if (!erased)
	{
		return false;
	}

	if (was_selected)
	{
		notify_selection_changed();
	}

	notify_snapshot_invalidated();
	return true;
}

void World::flush_destroy_requests()
{
	if (pending_destroy_objects_.empty())
	{
		return;
	}

	std::vector<WorldObject*> destroy_queue;
	destroy_queue.swap(pending_destroy_objects_);

	for (WorldObject* object : destroy_queue)
	{
		destroy_object_immediate(object);
	}
}

bool World::is_destroy_queued(const WorldObject* object) const
{
	return std::find(pending_destroy_objects_.begin(), pending_destroy_objects_.end(), object) != pending_destroy_objects_.end();
}
