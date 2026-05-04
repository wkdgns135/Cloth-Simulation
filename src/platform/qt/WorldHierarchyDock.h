#pragma once

#include <QDockWidget>

class QComboBox;
class QDoubleSpinBox;
class QListWidget;
class QSpinBox;
class ClothWorldController;
class WorldEditorController;

class WorldHierarchyDock : public QDockWidget
{
	Q_OBJECT

public:
	explicit WorldHierarchyDock(WorldEditorController& controller, ClothWorldController& cloth_world_controller, QWidget* parent = nullptr);
	~WorldHierarchyDock() override = default;

private:
	void build_ui();
	void refresh();
	void create_grid_object();
	void create_mesh_object();
	void spawn_sphere_object();

	WorldEditorController& controller_;
	ClothWorldController& cloth_world_controller_;
	QListWidget* object_list_widget_ = nullptr;
	QComboBox* new_solver_combo_ = nullptr;
	QSpinBox* new_grid_width_spin_ = nullptr;
	QSpinBox* new_grid_height_spin_ = nullptr;
	QDoubleSpinBox* new_grid_spacing_spin_ = nullptr;
};
