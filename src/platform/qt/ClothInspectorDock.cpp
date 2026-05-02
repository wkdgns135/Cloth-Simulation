#include "platform/qt/ClothInspectorDock.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

#include "platform/qt/ClothEditorController.h"

namespace
{
void configure_double_spin_box(QDoubleSpinBox* spin_box, double minimum, double maximum, double step, int decimals)
{
	spin_box->setRange(minimum, maximum);
	spin_box->setSingleStep(step);
	spin_box->setDecimals(decimals);
	spin_box->setKeyboardTracking(false);
}

const ClothEditorController::PropertyViewState* find_selected_property(
	const ClothEditorController::ClothViewState& cloth,
	const QString& property_id)
{
	for (const ClothEditorController::PropertyViewState& property : cloth.properties)
	{
		if (property.id == property_id)
		{
			return &property;
		}
	}

	return nullptr;
}
}

ClothInspectorDock::ClothInspectorDock(ClothEditorController& controller, QWidget* parent)
	: QDockWidget("Inspector", parent)
	, controller_(controller)
{
	setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	build_ui();

	connect(&controller_, &ClothEditorController::snapshot_updated, this, [this]() {
		refresh();
	});
	connect(&controller_, &ClothEditorController::selected_cloth_value_updated, this, [this](const QString& value_id) {
		if (value_id == "position")
		{
			refresh_transform_values();
			return;
		}

		if (value_id == "anchors_enabled")
		{
			refresh_action_values();
			return;
		}

		if (value_id == "damping" || value_id == "constraint_iterations")
		{
			refresh_simulation_values();
			return;
		}

		refresh();
	});

	refresh();
}

void ClothInspectorDock::build_ui()
{
	inspector_panel_ = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(inspector_panel_);
	layout->setContentsMargins(8, 8, 8, 8);

	QGroupBox* info_group = new QGroupBox("Selection", inspector_panel_);
	QFormLayout* info_layout = new QFormLayout(info_group);
	selected_name_label_ = new QLabel("-", info_group);
	selected_source_label_ = new QLabel("-", info_group);
	selected_solver_label_ = new QLabel("-", info_group);
	selected_topology_label_ = new QLabel("-", info_group);
	info_layout->addRow("Name", selected_name_label_);
	info_layout->addRow("Source", selected_source_label_);
	info_layout->addRow("Solver", selected_solver_label_);
	info_layout->addRow("Topology", selected_topology_label_);
	layout->addWidget(info_group);

	QGroupBox* transform_group = new QGroupBox("Transform", inspector_panel_);
	QFormLayout* transform_layout = new QFormLayout(transform_group);
	position_x_spin_ = new QDoubleSpinBox(transform_group);
	position_y_spin_ = new QDoubleSpinBox(transform_group);
	position_z_spin_ = new QDoubleSpinBox(transform_group);
	configure_double_spin_box(position_x_spin_, -1000.0, 1000.0, 0.05, 3);
	configure_double_spin_box(position_y_spin_, -1000.0, 1000.0, 0.05, 3);
	configure_double_spin_box(position_z_spin_, -1000.0, 1000.0, 0.05, 3);
	transform_layout->addRow("X", position_x_spin_);
	transform_layout->addRow("Y", position_y_spin_);
	transform_layout->addRow("Z", position_z_spin_);
	layout->addWidget(transform_group);

	connect(position_x_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
		update_selected_cloth_position();
	});
	connect(position_y_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
		update_selected_cloth_position();
	});
	connect(position_z_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
		update_selected_cloth_position();
	});

	QGroupBox* simulation_group = new QGroupBox("Simulation", inspector_panel_);
	QFormLayout* simulation_layout = new QFormLayout(simulation_group);
	damping_spin_ = new QDoubleSpinBox(simulation_group);
	configure_double_spin_box(damping_spin_, 0.0, 1.0, 0.01, 3);
	constraint_iterations_spin_ = new QSpinBox(simulation_group);
	constraint_iterations_spin_->setRange(0, 256);
	simulation_layout->addRow("Damping", damping_spin_);
	simulation_layout->addRow("Iterations", constraint_iterations_spin_);
	layout->addWidget(simulation_group);

	connect(damping_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
		update_selected_cloth_property("damping", static_cast<float>(damping_spin_->value()));
	});
	connect(constraint_iterations_spin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
		update_selected_cloth_property("constraint_iterations", constraint_iterations_spin_->value());
	});

	QGroupBox* actions_group = new QGroupBox("Actions", inspector_panel_);
	QVBoxLayout* actions_layout = new QVBoxLayout(actions_group);
	QPushButton* reset_cloth_button = new QPushButton("Reset", actions_group);
	toggle_anchors_button_ = new QPushButton("Toggle Anchors", actions_group);
	QPushButton* delete_cloth_button = new QPushButton("Delete", actions_group);
	actions_layout->addWidget(reset_cloth_button);
	actions_layout->addWidget(toggle_anchors_button_);
	actions_layout->addWidget(delete_cloth_button);
	layout->addWidget(actions_group);
	layout->addStretch(1);

	connect(reset_cloth_button, &QPushButton::clicked, this, [this]() {
		controller_.reset_selected_cloth();
	});
	connect(toggle_anchors_button_, &QPushButton::clicked, this, [this]() {
		controller_.toggle_selected_cloth_anchors();
	});
	connect(delete_cloth_button, &QPushButton::clicked, this, [this]() {
		controller_.delete_selected_cloth();
	});

	setWidget(inspector_panel_);
}

void ClothInspectorDock::refresh()
{
	const ClothEditorController::ClothViewState* cloth = controller_.selected_cloth();
	inspector_panel_->setEnabled(cloth != nullptr);

	if (!cloth)
	{
		selected_name_label_->setText("-");
		selected_source_label_->setText("-");
		selected_solver_label_->setText("-");
		selected_topology_label_->setText("-");
		toggle_anchors_button_->setText("Toggle Anchors");
		return;
	}

	selected_name_label_->setText(cloth->name);
	selected_source_label_->setText(QString("%1  |  %2").arg(cloth->source_kind_label, cloth->source_label));
	selected_solver_label_->setText(cloth->solver_label);
	selected_topology_label_->setText(QString("%1 particles / %2 triangles")
		.arg(cloth->particle_count)
		.arg(cloth->triangle_count));
	refresh_transform_values();
	refresh_simulation_values();
	refresh_action_values();
}

void ClothInspectorDock::update_selected_cloth_position()
{
	controller_.update_selected_cloth_position(
		static_cast<float>(position_x_spin_->value()),
		static_cast<float>(position_y_spin_->value()),
		static_cast<float>(position_z_spin_->value()));
}

void ClothInspectorDock::update_selected_cloth_property(const QString& property_id, const PropertyValue& value)
{
	const ClothEditorController::ClothViewState* cloth = controller_.selected_cloth();
	if (!cloth)
	{
		return;
	}

	const ClothEditorController::PropertyViewState* property = find_selected_property(*cloth, property_id);
	if (!property)
	{
		return;
	}

	controller_.update_selected_cloth_property(property->source_object_id, property->id, value);
}

void ClothInspectorDock::refresh_transform_values()
{
	const ClothEditorController::ClothViewState* cloth = controller_.selected_cloth();
	if (!cloth)
	{
		return;
	}

	const QSignalBlocker block_x(position_x_spin_);
	const QSignalBlocker block_y(position_y_spin_);
	const QSignalBlocker block_z(position_z_spin_);

	position_x_spin_->setValue(cloth->position_x);
	position_y_spin_->setValue(cloth->position_y);
	position_z_spin_->setValue(cloth->position_z);
}

void ClothInspectorDock::refresh_simulation_values()
{
	const ClothEditorController::ClothViewState* cloth = controller_.selected_cloth();
	if (!cloth)
	{
		return;
	}

	const QSignalBlocker block_damping(damping_spin_);
	const QSignalBlocker block_iterations(constraint_iterations_spin_);

	damping_spin_->setValue(cloth->damping);
	constraint_iterations_spin_->setValue(cloth->constraint_iterations);
}

void ClothInspectorDock::refresh_action_values()
{
	const ClothEditorController::ClothViewState* cloth = controller_.selected_cloth();
	if (!cloth)
	{
		toggle_anchors_button_->setText("Toggle Anchors");
		return;
	}

	toggle_anchors_button_->setText(cloth->anchors_enabled ? "Disable Anchors" : "Enable Anchors");
}
