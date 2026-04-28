#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "engine/core/Component.h"

enum class InputKey
{
	Unknown = 0,
	W,
	A,
	S,
	D,
	Q,
	E,
	R,
	Space,
	Up,
	Down,
	Left,
	Right,
};

enum class PointerButton
{
	None = 0,
	Left,
	Right,
	Middle,
};

constexpr std::uint32_t kInputModifierShift = 1u << 0;
constexpr std::uint32_t kInputModifierControl = 1u << 1;
constexpr std::uint32_t kInputModifierAlt = 1u << 2;

constexpr std::size_t kInputKeyCount = static_cast<std::size_t>(InputKey::Right) + 1;
constexpr std::size_t kPointerButtonCount = static_cast<std::size_t>(PointerButton::Middle) + 1;

struct PointerPosition
{
	float x = 0.0f;
	float y = 0.0f;
};

struct KeyInputEvent
{
	InputKey key = InputKey::Unknown;
	std::uint32_t modifiers = 0;
	bool is_auto_repeat = false;
};

struct PointerInputEvent
{
	PointerButton button = PointerButton::None;
	PointerPosition position;
	PointerPosition delta;
	std::uint32_t modifiers = 0;
};

struct ClickInputEvent
{
	PointerButton button = PointerButton::None;
	PointerPosition position;
	std::uint32_t modifiers = 0;
};

struct WheelInputEvent
{
	float steps = 0.0f;
	PointerPosition position;
	std::uint32_t modifiers = 0;
};

struct InputSubscriptionHandle
{
	std::uint64_t id = 0;

	[[nodiscard]] bool is_valid() const
	{
		return id != 0;
	}
};

class World;

class InputComponent : public Component
{
public:
	using KeyHandler = std::function<bool(const KeyInputEvent&)>;
	using PointerHandler = std::function<bool(const PointerInputEvent&)>;
	using WheelHandler = std::function<bool(const WheelInputEvent&)>;

	struct KeyBinding
	{
		InputKey key = InputKey::Unknown;
		KeyHandler handler;
	};

	struct PointerBinding
	{
		PointerButton button = PointerButton::None;
		PointerHandler handler;
	};

	~InputComponent() override = default;

	void awake() override;
	void destroy() override;

protected:
	void bind_key_pressed(InputKey key, KeyHandler handler);
	void bind_key_released(InputKey key, KeyHandler handler);
	void bind_pointer_pressed(PointerButton button, PointerHandler handler);
	void bind_pointer_released(PointerButton button, PointerHandler handler);
	void bind_pointer_moved(PointerHandler handler);
	void bind_wheel_scrolled(WheelHandler handler);

	template <typename T>
	void bind_key_pressed(InputKey key, T* instance, bool (T::*handler)(const KeyInputEvent&))
	{
		bind_key_pressed(key, [instance, handler](const KeyInputEvent& event) {
			return (instance->*handler)(event);
		});
	}

	template <typename T>
	void bind_key_released(InputKey key, T* instance, bool (T::*handler)(const KeyInputEvent&))
	{
		bind_key_released(key, [instance, handler](const KeyInputEvent& event) {
			return (instance->*handler)(event);
		});
	}

	template <typename T>
	void bind_pointer_pressed(PointerButton button, T* instance, bool (T::*handler)(const PointerInputEvent&))
	{
		bind_pointer_pressed(button, [instance, handler](const PointerInputEvent& event) {
			return (instance->*handler)(event);
		});
	}

	template <typename T>
	void bind_pointer_released(PointerButton button, T* instance, bool (T::*handler)(const PointerInputEvent&))
	{
		bind_pointer_released(button, [instance, handler](const PointerInputEvent& event) {
			return (instance->*handler)(event);
		});
	}

	template <typename T>
	void bind_pointer_moved(T* instance, bool (T::*handler)(const PointerInputEvent&))
	{
		bind_pointer_moved([instance, handler](const PointerInputEvent& event) {
			return (instance->*handler)(event);
		});
	}

	template <typename T>
	void bind_wheel_scrolled(T* instance, bool (T::*handler)(const WheelInputEvent&))
	{
		bind_wheel_scrolled([instance, handler](const WheelInputEvent& event) {
			return (instance->*handler)(event);
		});
	}

private:
	struct KeySubscription
	{
		InputKey key = InputKey::Unknown;
		InputSubscriptionHandle handle;
	};

	struct PointerSubscription
	{
		PointerButton button = PointerButton::None;
		InputSubscriptionHandle handle;
	};

	std::vector<KeyBinding> key_pressed_bindings_;
	std::vector<KeyBinding> key_released_bindings_;
	std::vector<PointerBinding> pointer_pressed_bindings_;
	std::vector<PointerBinding> pointer_released_bindings_;
	std::vector<PointerHandler> pointer_moved_handlers_;
	std::vector<WheelHandler> wheel_scrolled_handlers_;
	std::vector<KeySubscription> key_pressed_subscriptions_;
	std::vector<KeySubscription> key_released_subscriptions_;
	std::vector<PointerSubscription> pointer_pressed_subscriptions_;
	std::vector<PointerSubscription> pointer_released_subscriptions_;
	std::vector<InputSubscriptionHandle> pointer_moved_subscriptions_;
	std::vector<InputSubscriptionHandle> wheel_scrolled_subscriptions_;
	bool registered_with_world_ = false;
};
