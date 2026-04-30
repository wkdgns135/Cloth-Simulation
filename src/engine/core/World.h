#pragma once

#include <array>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/components/InputComponent.h"
#include "engine/core/IndexedStore.h"
#include "engine/core/WorldObject.h"
#include "engine/render/RenderScene.h"

class CameraObject;
class DirectionalLightObject;
class InputComponent;

class World : public Object
{
public:
	World();
	~World() override = default;

	World(const World&) = delete;
	World& operator=(const World&) = delete;

	void awake() override;
	void start() override;
	void update(float delta_time) override;
	void stop() override;
	void destroy() override;

	template <typename T, typename... Args>
	T& create_object(Args&&... args)
	{
		static_assert(std::is_base_of_v<WorldObject, T>);

		auto object = std::make_unique<T>(std::forward<Args>(args)...);
		object->set_world(this);
		T& result = *object;

		world_objects_.insert(std::move(object));

		if (awakened_)
		{
			result.awake();
		}
		if (started_)
		{
			result.start();
		}

		return result;
	}

	template <typename T>
	std::vector<T*> get_objects()
	{
		static_assert(std::is_base_of_v<WorldObject, T>);

		std::vector<T*> result;
		result.reserve(world_objects_.size());

		for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
		{
			if (T* typed_object = dynamic_cast<T*>(object.get()))
			{
				result.push_back(typed_object);
			}
		}

		return result;
	}

	template <typename T>
	std::vector<const T*> get_objects() const
	{
		static_assert(std::is_base_of_v<WorldObject, T>);

		std::vector<const T*> result;
		result.reserve(world_objects_.size());

		for (const std::unique_ptr<WorldObject>& object : world_objects_.ordered())
		{
			if (const T* typed_object = dynamic_cast<const T*>(object.get()))
			{
				result.push_back(typed_object);
			}
		}

		return result;
	}

	template <typename T = WorldObject>
	T* find_object(ObjectId object_id)
	{
		static_assert(std::is_base_of_v<WorldObject, T>);
		return dynamic_cast<T*>(world_objects_.find(object_id));
	}

	template <typename T = WorldObject>
	const T* find_object(ObjectId object_id) const
	{
		static_assert(std::is_base_of_v<WorldObject, T>);
		return dynamic_cast<const T*>(world_objects_.find(object_id));
	}

	bool on_key_pressed(const KeyInputEvent& event);
	bool on_key_released(const KeyInputEvent& event);
	bool on_pointer_pressed(const PointerInputEvent& event);
	bool on_pointer_released(const PointerInputEvent& event);
	bool on_pointer_moved(const PointerInputEvent& event);
	bool on_click(const ClickInputEvent& event);
	bool on_wheel_scrolled(const WheelInputEvent& event);
	void set_viewport_size(int width, int height);
	void request_destroy_object(WorldObject* object);

	RenderScene build_render_scene() const;

protected:
	virtual bool native_on_key_pressed(const KeyInputEvent& event);
	virtual bool native_on_key_released(const KeyInputEvent& event);
	virtual bool native_on_pointer_pressed(const PointerInputEvent& event);
	virtual bool native_on_pointer_released(const PointerInputEvent& event);
	virtual bool native_on_pointer_moved(const PointerInputEvent& event);
	virtual bool native_on_click(const ClickInputEvent& event);
	virtual bool native_on_wheel_scrolled(const WheelInputEvent& event);
	bool destroy_object(WorldObject* object);

	IndexedStore<WorldObject> world_objects_;
	bool awakened_ = false;
	bool started_ = false;
	bool destroyed_ = false;
	CameraObject* main_camera_object_ = nullptr;
	DirectionalLightObject* main_directional_light_object_ = nullptr;

private:
	template <typename Handler>
	struct InputListenerEntry
	{
		InputSubscriptionHandle handle;
		Handler handler;
	};

	InputSubscriptionHandle subscribe_key_pressed(InputKey key, InputComponent::KeyHandler handler);
	InputSubscriptionHandle subscribe_key_released(InputKey key, InputComponent::KeyHandler handler);
	InputSubscriptionHandle subscribe_pointer_pressed(PointerButton button, InputComponent::PointerHandler handler);
	InputSubscriptionHandle subscribe_pointer_released(PointerButton button, InputComponent::PointerHandler handler);
	InputSubscriptionHandle subscribe_pointer_moved(InputComponent::PointerHandler handler);
	InputSubscriptionHandle subscribe_wheel_scrolled(InputComponent::WheelHandler handler);

	void unsubscribe_key_pressed(InputKey key, InputSubscriptionHandle handle);
	void unsubscribe_key_released(InputKey key, InputSubscriptionHandle handle);
	void unsubscribe_pointer_pressed(PointerButton button, InputSubscriptionHandle handle);
	void unsubscribe_pointer_released(PointerButton button, InputSubscriptionHandle handle);
	void unsubscribe_pointer_moved(InputSubscriptionHandle handle);
	void unsubscribe_wheel_scrolled(InputSubscriptionHandle handle);
	WorldObject* pick_object(const PointerPosition& position) const;
	void update_hovered_object(const PointerPosition& position);
	bool destroy_object_immediate(WorldObject* object);
	void flush_destroy_requests();
	bool is_destroy_queued(const WorldObject* object) const;

	std::array<std::vector<InputListenerEntry<InputComponent::KeyHandler>>, kInputKeyCount> key_pressed_listeners_;
	std::array<std::vector<InputListenerEntry<InputComponent::KeyHandler>>, kInputKeyCount> key_released_listeners_;
	std::array<std::vector<InputListenerEntry<InputComponent::PointerHandler>>, kPointerButtonCount> pointer_pressed_listeners_;
	std::array<std::vector<InputListenerEntry<InputComponent::PointerHandler>>, kPointerButtonCount> pointer_released_listeners_;
	std::vector<InputListenerEntry<InputComponent::PointerHandler>> pointer_moved_listeners_;
	std::vector<InputListenerEntry<InputComponent::WheelHandler>> wheel_scrolled_listeners_;
	std::uint64_t next_input_subscription_id_ = 1;
	WorldObject* hovered_object_ = nullptr;
	int viewport_width_ = 1;
	int viewport_height_ = 1;
	bool updating_ = false;
	std::vector<WorldObject*> pending_destroy_objects_;

	friend class InputComponent;
};
