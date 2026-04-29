#pragma once

#include <QMainWindow>

class ClothEditorController;
class ClothHierarchyDock;
class ClothInspectorDock;
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
	ClothEditorController* cloth_editor_controller_ = nullptr;
	ClothHierarchyDock* cloth_hierarchy_dock_ = nullptr;
	ClothInspectorDock* cloth_inspector_dock_ = nullptr;
};
