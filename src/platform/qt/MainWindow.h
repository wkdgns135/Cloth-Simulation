#pragma once

#include <QMainWindow>

class Engine;
class RenderSystem;
class ViewportWidget;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(RenderSystem& render_system, Engine& engine, QWidget* parent = nullptr);
	~MainWindow() override = default;

	void request_render();

private:
	ViewportWidget* viewport_widget_ = nullptr;
};
