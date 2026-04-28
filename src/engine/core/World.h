#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/lifecycle/Lifecycle.h"
#include "engine/core/Object.h"
#include "engine/render/RenderScene.h"

class CameraObject;
class DirectionalLightObject;
struct KeyInputEvent;
struct PointerInputEvent;
struct ClickInputEvent;
struct WheelInputEvent;

class World : public Lifecycle
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
		static_assert(std::is_base_of_v<Object, T>);

		auto object = std::make_unique<T>(std::forward<Args>(args)...);
		T& result = *object;

		objects_.push_back(std::move(object));

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

	bool handle_key_pressed(const KeyInputEvent& event);
	bool handle_key_released(const KeyInputEvent& event);
	bool handle_pointer_pressed(const PointerInputEvent& event);
	bool handle_pointer_released(const PointerInputEvent& event);
	bool handle_pointer_moved(const PointerInputEvent& event);
	bool handle_click(const ClickInputEvent& event);
	bool handle_wheel_scrolled(const WheelInputEvent& event);

	RenderScene build_render_scene() const;

	CameraObject& main_camera();
	const CameraObject& main_camera() const;

	DirectionalLightObject& main_directional_light();
	const DirectionalLightObject& main_directional_light() const;

	void set_main_camera(CameraObject& camera_object);
	void set_main_directional_light(DirectionalLightObject& directional_light_object);

private:
	std::vector<std::unique_ptr<Object>> objects_;
	CameraObject* main_camera_object_ = nullptr;
	DirectionalLightObject* main_directional_light_object_ = nullptr;
	bool awakened_ = false;
	bool started_ = false;
	bool destroyed_ = false;
};
