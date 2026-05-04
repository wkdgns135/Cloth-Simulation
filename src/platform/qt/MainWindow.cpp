#include "platform/qt/MainWindow.h"

#include "engine/Engine.h"
#include "engine/render/RenderSystem.h"
#include "platform/qt/ClothWorldController.h"
#include "platform/qt/WorldEditorController.h"
#include "platform/qt/WorldHierarchyDock.h"
#include "platform/qt/WorldInspectorDock.h"
#include "platform/qt/ViewportWidget.h"

MainWindow::MainWindow(RenderSystem& render_system, Engine& engine, QWidget* parent)
	: QMainWindow(parent)
{
	resize(1440, 860);
	setWindowTitle("PBD Cloth Simulator");

	viewport_widget_ = new ViewportWidget(render_system, engine, this);
	setCentralWidget(viewport_widget_);

	cloth_world_controller_ = new ClothWorldController(engine, this);
	world_editor_controller_ = new WorldEditorController(engine, this);

	world_hierarchy_dock_ = new WorldHierarchyDock(*world_editor_controller_, *cloth_world_controller_, this);
	addDockWidget(Qt::LeftDockWidgetArea, world_hierarchy_dock_);

	world_inspector_dock_ = new WorldInspectorDock(*world_editor_controller_, *cloth_world_controller_, this);
	addDockWidget(Qt::RightDockWidgetArea, world_inspector_dock_);
}

void MainWindow::request_render()
{
	if (viewport_widget_)
	{
		viewport_widget_->update();
	}
}
