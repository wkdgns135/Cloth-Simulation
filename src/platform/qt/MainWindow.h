#pragma once

#include <QMainWindow>

class ClothWorldController;
class WorldEditorController;
class WorldHierarchyDock;
class WorldInspectorDock;
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
	ClothWorldController* cloth_world_controller_ = nullptr;
	WorldEditorController* world_editor_controller_ = nullptr;
	WorldHierarchyDock* world_hierarchy_dock_ = nullptr;
	WorldInspectorDock* world_inspector_dock_ = nullptr;
};
