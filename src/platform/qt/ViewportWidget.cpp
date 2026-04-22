#include "platform/qt/ViewportWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>

#include "engine/Engine.h"
#include "engine/core/World.h"
#include "engine/render/RenderSystem.h"

namespace
{
constexpr float kCameraMoveSpeed = 1.2f;
constexpr float kCameraOrbitSensitivity = 0.006f;
constexpr float kCameraZoomStepScale = 0.88f;
constexpr float kMinCameraDistance = 0.2f;
constexpr float kMaxCameraDistance = 20.0f;
constexpr float kMaxCameraPitch = 1.55334306f;
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

ViewportWidget::ViewportWidget(RenderSystem& render_system, Engine& engine, QWidget* parent)
	: QOpenGLWidget(parent)
	, render_system_(render_system)
	, engine_(engine)
{
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	camera_time_.start();
	camera_timer_.setTimerType(Qt::PreciseTimer);
	connect(&camera_timer_, &QTimer::timeout, this, &ViewportWidget::update_camera_movement);
	camera_timer_.start(16);
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

	switch (event->key())
	{
	case Qt::Key_W:
	case Qt::Key_Up:
		moving_forward_ = true;
		event->accept();
		return;
	case Qt::Key_S:
	case Qt::Key_Down:
		moving_backward_ = true;
		event->accept();
		return;
	case Qt::Key_A:
	case Qt::Key_Left:
		moving_left_ = true;
		event->accept();
		return;
	case Qt::Key_D:
	case Qt::Key_Right:
		moving_right_ = true;
		event->accept();
		return;
	case Qt::Key_E:
		moving_up_ = true;
		event->accept();
		return;
	case Qt::Key_Q:
		moving_down_ = true;
		event->accept();
		return;
	default:
		QOpenGLWidget::keyPressEvent(event);
		return;
	}
}

void ViewportWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
	{
		event->accept();
		return;
	}

	switch (event->key())
	{
	case Qt::Key_W:
	case Qt::Key_Up:
		moving_forward_ = false;
		event->accept();
		return;
	case Qt::Key_S:
	case Qt::Key_Down:
		moving_backward_ = false;
		event->accept();
		return;
	case Qt::Key_A:
	case Qt::Key_Left:
		moving_left_ = false;
		event->accept();
		return;
	case Qt::Key_D:
	case Qt::Key_Right:
		moving_right_ = false;
		event->accept();
		return;
	case Qt::Key_E:
		moving_up_ = false;
		event->accept();
		return;
	case Qt::Key_Q:
		moving_down_ = false;
		event->accept();
		return;
	default:
		QOpenGLWidget::keyReleaseEvent(event);
		return;
	}
}

void ViewportWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		setFocus();
		rotating_camera_ = true;
		last_mouse_position_ = event->position().toPoint();
		event->accept();
		return;
	}

	QOpenGLWidget::mousePressEvent(event);
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		rotating_camera_ = false;
		event->accept();
		return;
	}

	QOpenGLWidget::mouseReleaseEvent(event);
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (rotating_camera_)
	{
		const QPoint current_position = event->position().toPoint();
		const QPoint delta = current_position - last_mouse_position_;
		last_mouse_position_ = current_position;

		orbit_camera(delta);
		update();
		event->accept();
		return;
	}

	QOpenGLWidget::mouseMoveEvent(event);
}

void ViewportWidget::wheelEvent(QWheelEvent* event)
{
	const float wheel_steps = static_cast<float>(event->angleDelta().y()) / 120.0f;
	if (wheel_steps != 0.0f)
	{
		zoom_camera(wheel_steps);
		update();
		event->accept();
		return;
	}

	QOpenGLWidget::wheelEvent(event);
}

void ViewportWidget::update_camera_movement()
{
	const qint64 elapsed_ms = camera_time_.restart();
	const float delta_time = std::min(static_cast<float>(elapsed_ms) / 1000.0f, 0.05f);

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
	update();
}

void ViewportWidget::move_camera(float delta_time)
{
	const bool moving_forward = moving_forward_;
	const bool moving_backward = moving_backward_;
	const bool moving_left = moving_left_;
	const bool moving_right = moving_right_;
	const bool moving_up = moving_up_;
	const bool moving_down = moving_down_;

	engine_.enqueue_world_job([=](World& current_world) {
		Camera& camera = current_world.camera();

		const glm::vec3 forward = safe_normalize(camera.target - camera.position, glm::vec3(0.0f, 0.0f, -1.0f));
		const glm::vec3 up = safe_normalize(camera.up, glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::vec3 right = safe_normalize(glm::cross(forward, up), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::vec3 movement(0.0f);
		if (moving_forward)
		{
			movement += forward;
		}
		if (moving_backward)
		{
			movement -= forward;
		}
		if (moving_right)
		{
			movement += right;
		}
		if (moving_left)
		{
			movement -= right;
		}
		if (moving_up)
		{
			movement += up;
		}
		if (moving_down)
		{
			movement -= up;
		}

		if (glm::dot(movement, movement) <= kVectorLengthThreshold)
		{
			return;
		}

		const glm::vec3 offset = glm::normalize(movement) * kCameraMoveSpeed * delta_time;
		camera.position += offset;
		camera.target += offset;
	});
}

void ViewportWidget::orbit_camera(const QPoint& delta)
{
	if (delta.isNull())
	{
		return;
	}

	engine_.enqueue_world_job([delta](World& current_world) {
		Camera& camera = current_world.camera();

		const glm::vec3 offset = camera.position - camera.target;
		const float radius = glm::length(offset);
		if (radius <= kMinCameraDistance)
		{
			return;
		}

		float yaw = std::atan2(offset.x, offset.z);
		float pitch = std::asin(std::clamp(offset.y / radius, -1.0f, 1.0f));

		yaw -= static_cast<float>(delta.x()) * kCameraOrbitSensitivity;
		pitch += static_cast<float>(delta.y()) * kCameraOrbitSensitivity;
		pitch = std::clamp(pitch, -kMaxCameraPitch, kMaxCameraPitch);

		const float cos_pitch = std::cos(pitch);
		camera.position = camera.target + glm::vec3(
			std::sin(yaw) * cos_pitch * radius,
			std::sin(pitch) * radius,
			std::cos(yaw) * cos_pitch * radius);
		camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	});
}

void ViewportWidget::zoom_camera(float wheel_steps)
{
	engine_.enqueue_world_job([wheel_steps](World& current_world) {
		Camera& camera = current_world.camera();

		glm::vec3 offset = camera.position - camera.target;
		float distance = glm::length(offset);

		if (distance <= kVectorLengthThreshold)
		{
			offset = glm::vec3(0.0f, 0.0f, 1.0f);
			distance = 1.0f;
		}

		const glm::vec3 direction = offset / distance;
		const float next_distance = std::clamp(
			distance * std::pow(kCameraZoomStepScale, wheel_steps),
			kMinCameraDistance,
			kMaxCameraDistance);

		camera.position = camera.target + direction * next_distance;
	});
}
