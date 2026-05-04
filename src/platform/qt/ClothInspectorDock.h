#pragma once

#include <cstdint>
#include <vector>

#include <QDockWidget>

#include "engine/core/Property.h"

class QDoubleSpinBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QVBoxLayout;
class QWidget;
class ClothEditorController;

class ClothInspectorDock : public QDockWidget
{
	Q_OBJECT

public:
	explicit ClothInspectorDock(ClothEditorController& controller, QWidget* parent = nullptr);
	~ClothInspectorDock() override = default;

private:
	struct PropertyEditorBinding
	{
		std::uint64_t source_object_id = 0;
		QString property_id;
		PropertyType type = PropertyType::Float;
		QWidget* editor = nullptr;
		QCheckBox* check_box = nullptr;
		QSpinBox* spin_box = nullptr;
		QDoubleSpinBox* double_spin_box = nullptr;
		QLineEdit* line_edit = nullptr;
		QDoubleSpinBox* vec3_x = nullptr;
		QDoubleSpinBox* vec3_y = nullptr;
		QDoubleSpinBox* vec3_z = nullptr;
	};

	void build_ui();
	void refresh();
	void refresh_property_values();
	void rebuild_property_editors();
	void clear_property_editors();
	void update_selected_object_property(std::uint64_t source_object_id, const QString& property_id, const PropertyValue& value);

	ClothEditorController& controller_;
	QLabel* selected_name_label_ = nullptr;
	QLabel* selected_source_label_ = nullptr;
	QLabel* selected_solver_label_ = nullptr;
	QLabel* selected_topology_label_ = nullptr;
	QWidget* inspector_panel_ = nullptr;
	QWidget* properties_panel_ = nullptr;
	QVBoxLayout* properties_layout_ = nullptr;
	std::vector<PropertyEditorBinding> property_editors_;
};
