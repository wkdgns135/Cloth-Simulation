#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>

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
	void dispatch_key_event(bool is_pressed, QKeyEvent* event);
	void dispatch_pointer_press_event(QMouseEvent* event);
	void dispatch_pointer_release_event(QMouseEvent* event);
	void dispatch_pointer_move_event(QMouseEvent* event, const QPoint& delta);
	void dispatch_click_event(QMouseEvent* event);
	void dispatch_wheel_event(QWheelEvent* event, float wheel_steps);

	RenderSystem& render_system_;
	Engine& engine_;
	QPoint last_mouse_position_;
	QPoint left_button_press_position_;
	bool left_button_dragged_ = false;
	bool has_last_mouse_position_ = false;
};
