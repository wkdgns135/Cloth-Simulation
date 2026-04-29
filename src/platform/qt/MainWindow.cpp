#include "platform/qt/MainWindow.h"

#include "engine/Engine.h"
#include "engine/render/RenderSystem.h"
#include "platform/qt/ClothEditorController.h"
#include "platform/qt/ClothHierarchyDock.h"
#include "platform/qt/ClothInspectorDock.h"
#include "platform/qt/ViewportWidget.h"

MainWindow::MainWindow(RenderSystem& render_system, Engine& engine, QWidget* parent)
	: QMainWindow(parent)
{
	resize(1440, 860);
	setWindowTitle("PBD Cloth Simulator");

	viewport_widget_ = new ViewportWidget(render_system, engine, this);
	setCentralWidget(viewport_widget_);

	cloth_editor_controller_ = new ClothEditorController(engine, this);

	cloth_hierarchy_dock_ = new ClothHierarchyDock(*cloth_editor_controller_, this);
	addDockWidget(Qt::LeftDockWidgetArea, cloth_hierarchy_dock_);

	cloth_inspector_dock_ = new ClothInspectorDock(*cloth_editor_controller_, this);
	addDockWidget(Qt::RightDockWidgetArea, cloth_inspector_dock_);
}

void MainWindow::request_render()
{
	if (viewport_widget_)
	{
		viewport_widget_->update();
	}
}
