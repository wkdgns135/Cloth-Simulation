#pragma once

#include <cstdint>

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

class InputComponent : public Component
{
public:
	~InputComponent() override = default;

	virtual bool on_key_pressed(const KeyInputEvent& event) { return false; }
	virtual bool on_key_released(const KeyInputEvent& event) { return false; }
	virtual bool on_pointer_pressed(const PointerInputEvent& event) { return false; }
	virtual bool on_pointer_released(const PointerInputEvent& event) { return false; }
	virtual bool on_pointer_moved(const PointerInputEvent& event) { return false; }
	virtual bool on_click(const ClickInputEvent& event) { return false; }
	virtual bool on_wheel_scrolled(const WheelInputEvent& event) { return false; }
};
