#include "platform/qt/ViewportWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include "engine/Engine.h"
#include "engine/components/InputComponent.h"
#include "engine/core/World.h"
#include "engine/render/RenderSystem.h"

namespace
{
constexpr int kClickMovementThresholdSquared = 9;

std::uint32_t to_input_modifiers(Qt::KeyboardModifiers modifiers)
{
	std::uint32_t result = 0;

	if (modifiers.testFlag(Qt::ShiftModifier))
	{
		result |= kInputModifierShift;
	}
	if (modifiers.testFlag(Qt::ControlModifier))
	{
		result |= kInputModifierControl;
	}
	if (modifiers.testFlag(Qt::AltModifier))
	{
		result |= kInputModifierAlt;
	}

	return result;
}

InputKey to_input_key(int key)
{
	switch (key)
	{
	case Qt::Key_W:
		return InputKey::W;
	case Qt::Key_A:
		return InputKey::A;
	case Qt::Key_S:
		return InputKey::S;
	case Qt::Key_D:
		return InputKey::D;
	case Qt::Key_Q:
		return InputKey::Q;
	case Qt::Key_E:
		return InputKey::E;
	case Qt::Key_R:
		return InputKey::R;
	case Qt::Key_Space:
		return InputKey::Space;
	case Qt::Key_Up:
		return InputKey::Up;
	case Qt::Key_Down:
		return InputKey::Down;
	case Qt::Key_Left:
		return InputKey::Left;
	case Qt::Key_Right:
		return InputKey::Right;
	default:
		return InputKey::Unknown;
	}
}

PointerButton to_pointer_button(Qt::MouseButton button)
{
	switch (button)
	{
	case Qt::LeftButton:
		return PointerButton::Left;
	case Qt::RightButton:
		return PointerButton::Right;
	case Qt::MiddleButton:
		return PointerButton::Middle;
	default:
		return PointerButton::None;
	}
}

PointerPosition to_pointer_position(const QPointF& point)
{
	return PointerPosition{
		static_cast<float>(point.x()),
		static_cast<float>(point.y()),
	};
}
}

ViewportWidget::ViewportWidget(RenderSystem& render_system, Engine& engine, QWidget* parent)
	: QOpenGLWidget(parent)
	, render_system_(render_system)
	, engine_(engine)
{
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
}

ViewportWidget::~ViewportWidget()
{
	if (context())
	{
		makeCurrent();
		render_system_.shutdown_gl();
		doneCurrent();
		return;
	}

	render_system_.shutdown_gl();
}

void ViewportWidget::initializeGL()
{
	initializeOpenGLFunctions();
	render_system_.initialize_gl();
}

void ViewportWidget::resizeGL(int w, int h)
{
	render_system_.resize(w, h);
	engine_.enqueue_world_job([w, h](World& current_world) {
		current_world.set_viewport_size(w, h);
	});
}

void ViewportWidget::paintGL()
{
	render_system_.render();
}

void ViewportWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
	{
		event->accept();
		return;
	}

	const InputKey key = to_input_key(event->key());
	if (key != InputKey::Unknown)
	{
		dispatch_key_event(true, event);
		event->accept();
		return;
	}

	QOpenGLWidget::keyPressEvent(event);
}

void ViewportWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
	{
		event->accept();
		return;
	}

	const InputKey key = to_input_key(event->key());
	if (key != InputKey::Unknown)
	{
		dispatch_key_event(false, event);
		event->accept();
		return;
	}

	QOpenGLWidget::keyReleaseEvent(event);
}

void ViewportWidget::mousePressEvent(QMouseEvent* event)
{
	dispatch_pointer_press_event(event);
	setFocus();

	const QPoint current_position = event->position().toPoint();
	last_mouse_position_ = current_position;
	has_last_mouse_position_ = true;

	if (event->button() == Qt::LeftButton)
	{
		left_button_press_position_ = current_position;
		left_button_dragged_ = false;
		event->accept();
		return;
	}

	QOpenGLWidget::mousePressEvent(event);
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* event)
{
	dispatch_pointer_release_event(event);

	if (event->button() == Qt::LeftButton)
	{
		const QPoint release_position = event->position().toPoint();
		const QPoint click_delta = release_position - left_button_press_position_;
		const int click_distance_squared = click_delta.x() * click_delta.x() + click_delta.y() * click_delta.y();
		if (!left_button_dragged_ && click_distance_squared <= kClickMovementThresholdSquared)
		{
			dispatch_click_event(event);
		}

		event->accept();
		return;
	}

	QOpenGLWidget::mouseReleaseEvent(event);
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event)
{
	const QPoint current_position = event->position().toPoint();
	const QPoint delta = has_last_mouse_position_ ? current_position - last_mouse_position_ : QPoint();
	last_mouse_position_ = current_position;
	has_last_mouse_position_ = true;

	dispatch_pointer_move_event(event, delta);

	if (event->buttons().testFlag(Qt::LeftButton) && !delta.isNull())
	{
		left_button_dragged_ = true;
	}

	QOpenGLWidget::mouseMoveEvent(event);
}

void ViewportWidget::wheelEvent(QWheelEvent* event)
{
	const float wheel_steps = static_cast<float>(event->angleDelta().y()) / 120.0f;
	if (wheel_steps != 0.0f)
	{
		dispatch_wheel_event(event, wheel_steps);
		event->accept();
		return;
	}

	QOpenGLWidget::wheelEvent(event);
}

void ViewportWidget::dispatch_key_event(bool is_pressed, QKeyEvent* event)
{
	const InputKey key = to_input_key(event->key());
	if (key == InputKey::Unknown)
	{
		return;
	}

	const KeyInputEvent input_event{
		key,
		to_input_modifiers(event->modifiers()),
		event->isAutoRepeat(),
	};

	engine_.enqueue_world_job([input_event, is_pressed](World& current_world) {
		if (is_pressed)
		{
			current_world.on_key_pressed(input_event);
			return;
		}

		current_world.on_key_released(input_event);
	});
}

void ViewportWidget::dispatch_pointer_press_event(QMouseEvent* event)
{
	const PointerButton button = to_pointer_button(event->button());
	if (button == PointerButton::None)
	{
		return;
	}

	const PointerInputEvent input_event{
		button,
		to_pointer_position(event->position()),
		{},
		to_input_modifiers(event->modifiers()),
	};

	engine_.enqueue_world_job([input_event](World& current_world) {
		current_world.on_pointer_pressed(input_event);
	});
}

void ViewportWidget::dispatch_pointer_release_event(QMouseEvent* event)
{
	const PointerButton button = to_pointer_button(event->button());
	if (button == PointerButton::None)
	{
		return;
	}

	const PointerInputEvent input_event{
		button,
		to_pointer_position(event->position()),
		{},
		to_input_modifiers(event->modifiers()),
	};

	engine_.enqueue_world_job([input_event](World& current_world) {
		current_world.on_pointer_released(input_event);
	});
}

void ViewportWidget::dispatch_pointer_move_event(QMouseEvent* event, const QPoint& delta)
{
	const PointerInputEvent input_event{
		PointerButton::None,
		to_pointer_position(event->position()),
		PointerPosition{
			static_cast<float>(delta.x()),
			static_cast<float>(delta.y()),
		},
		to_input_modifiers(event->modifiers()),
	};

	engine_.enqueue_world_job([input_event](World& current_world) {
		current_world.on_pointer_moved(input_event);
	});
}

void ViewportWidget::dispatch_click_event(QMouseEvent* event)
{
	const PointerButton button = to_pointer_button(event->button());
	if (button == PointerButton::None)
	{
		return;
	}

	const ClickInputEvent input_event{
		button,
		to_pointer_position(event->position()),
		to_input_modifiers(event->modifiers()),
	};

	engine_.enqueue_world_job([input_event](World& current_world) {
		current_world.on_click(input_event);
	});
}

void ViewportWidget::dispatch_wheel_event(QWheelEvent* event, float wheel_steps)
{
	const WheelInputEvent input_event{
		wheel_steps,
		to_pointer_position(event->position()),
		to_input_modifiers(event->modifiers()),
	};

	engine_.enqueue_world_job([input_event](World& current_world) {
		current_world.on_wheel_scrolled(input_event);
	});
}
