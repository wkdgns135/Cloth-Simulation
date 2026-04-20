#pragma once

#include <QMainWindow>

class ClothRenderSystem;
class ViewportWidget;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(ClothRenderSystem& render_system, QWidget* parent = nullptr);
	~MainWindow() override = default;

	void request_render();

private:
	ViewportWidget* viewport_widget_ = nullptr;
};
