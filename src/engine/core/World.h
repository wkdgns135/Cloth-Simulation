#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/lifecycle/Lifecycle.h"
#include "engine/core/Object.h"
#include "engine/render/RenderScene.h"

struct KeyInputEvent;
struct PointerInputEvent;
struct ClickInputEvent;
struct WheelInputEvent;

class World : public Lifecycle
{
public:
	World() = default;
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

	Camera& camera() { return camera_; }
	const Camera& camera() const { return camera_; }

	DirectionalLight& directional_light() { return directional_light_; }
	const DirectionalLight& directional_light() const { return directional_light_; }

private:
	std::vector<std::unique_ptr<Object>> objects_;
	Camera camera_;
	DirectionalLight directional_light_;
	bool awakened_ = false;
	bool started_ = false;
	bool destroyed_ = false;
};
