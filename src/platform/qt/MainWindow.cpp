#include "platform/qt/MainWindow.h"

#include "engine/Engine.h"
#include "engine/render/RenderSystem.h"
#include "platform/qt/ViewportWidget.h"

MainWindow::MainWindow(RenderSystem& render_system, Engine& engine, QWidget* parent)
	: QMainWindow(parent)
{
	resize(1280, 720);
	setWindowTitle("PBD Cloth Simulator");

	viewport_widget_ = new ViewportWidget(render_system, engine, this);
	setCentralWidget(viewport_widget_);
}

void MainWindow::request_render()
{
	if (viewport_widget_)
	{
		viewport_widget_->update();
	}
}
