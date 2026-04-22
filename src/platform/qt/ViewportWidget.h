#pragma once

#include <QElapsedTimer>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>
#include <QTimer>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class Engine;
class RenderSystem;

class ViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	ViewportWidget(RenderSystem& render_system, Engine& engine, QWidget* parent = nullptr);
	~ViewportWidget() override;

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void update_camera_movement();
	void move_camera(float delta_time);
	void orbit_camera(const QPoint& delta);
	void zoom_camera(float wheel_steps);

	RenderSystem& render_system_;
	Engine& engine_;
	QTimer camera_timer_;
	QElapsedTimer camera_time_;
	QPoint last_mouse_position_;
	bool moving_forward_ = false;
	bool moving_backward_ = false;
	bool moving_left_ = false;
	bool moving_right_ = false;
	bool moving_up_ = false;
	bool moving_down_ = false;
	bool rotating_camera_ = false;
};
