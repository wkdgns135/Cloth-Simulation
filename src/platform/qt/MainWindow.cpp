#include "platform/qt/MainWindow.h"

#include "cloth/render/ClothRenderSystem.h"
#include "platform/qt/ViewportWidget.h"

MainWindow::MainWindow(ClothRenderSystem& render_system, QWidget* parent)
	: QMainWindow(parent)
{
	resize(1280, 720);
	setWindowTitle("PBD Cloth Simulator");

	viewport_widget_ = new ViewportWidget(render_system, this);
	setCentralWidget(viewport_widget_);
}

void MainWindow::request_render()
{
	if (viewport_widget_)
	{
		viewport_widget_->update();
	}
}
