#pragma once

#include <QDockWidget>

class QComboBox;
class QDoubleSpinBox;
class QListWidget;
class QSpinBox;
class ClothEditorController;

class ClothHierarchyDock : public QDockWidget
{
	Q_OBJECT

public:
	explicit ClothHierarchyDock(ClothEditorController& controller, QWidget* parent = nullptr);
	~ClothHierarchyDock() override = default;

private:
	void build_ui();
	void refresh();
	void create_grid_cloth();
	void create_mesh_cloth();
	void spawn_sphere();

	ClothEditorController& controller_;
	QListWidget* cloth_list_widget_ = nullptr;
	QComboBox* new_solver_combo_ = nullptr;
	QSpinBox* new_grid_width_spin_ = nullptr;
	QSpinBox* new_grid_height_spin_ = nullptr;
	QDoubleSpinBox* new_grid_spacing_spin_ = nullptr;
};
