#pragma once

#include <QDockWidget>

#include "engine/core/Property.h"

class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QWidget;
class ClothEditorController;

class ClothInspectorDock : public QDockWidget
{
	Q_OBJECT

public:
	explicit ClothInspectorDock(ClothEditorController& controller, QWidget* parent = nullptr);
	~ClothInspectorDock() override = default;

private:
	void build_ui();
	void refresh();
	void refresh_transform_values();
	void refresh_simulation_values();
	void refresh_action_values();
	void update_selected_cloth_position();
	void update_selected_cloth_property(const QString& property_id, const PropertyValue& value);

	ClothEditorController& controller_;
	QLabel* selected_name_label_ = nullptr;
	QLabel* selected_source_label_ = nullptr;
	QLabel* selected_solver_label_ = nullptr;
	QLabel* selected_topology_label_ = nullptr;
	QDoubleSpinBox* position_x_spin_ = nullptr;
	QDoubleSpinBox* position_y_spin_ = nullptr;
	QDoubleSpinBox* position_z_spin_ = nullptr;
	QDoubleSpinBox* damping_spin_ = nullptr;
	QSpinBox* constraint_iterations_spin_ = nullptr;
	QPushButton* toggle_anchors_button_ = nullptr;
	QWidget* inspector_panel_ = nullptr;
};
