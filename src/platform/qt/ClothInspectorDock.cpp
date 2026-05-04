#include "platform/qt/ClothInspectorDock.h"

#include <cmath>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
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
	const ClothEditorController::ObjectViewState& object,
	std::uint64_t source_object_id,
	const QString& property_id)
{
	for (const ClothEditorController::PropertyViewState& property : object.properties)
	{
		if (property.source_object_id == source_object_id && property.id == property_id)
		{
			return &property;
		}
	}

	return nullptr;
}

QString property_section_title(const ClothEditorController::PropertyViewState& property)
{
	if (property.source_label.isEmpty())
	{
		return property.group;
	}

	if (property.group.isEmpty())
	{
		return property.source_label;
	}

	return QString("%1  |  %2").arg(property.source_label, property.group);
}

double property_double_or(const std::optional<PropertyValue>& value, double fallback)
{
	float converted_value = 0.0f;
	return value && convert_property_value(*value, converted_value) ? static_cast<double>(converted_value) : fallback;
}

int property_int_or(const std::optional<PropertyValue>& value, int fallback)
{
	int converted_value = 0;
	return value && convert_property_value(*value, converted_value) ? converted_value : fallback;
}

int decimals_for_step(const std::optional<PropertyValue>& step)
{
	const double step_value = property_double_or(step, 0.01);
	double scaled_step = std::abs(step_value);
	int decimals = 0;

	while (decimals < 6 && scaled_step > 0.0 && std::abs(std::round(scaled_step) - scaled_step) > 0.000001)
	{
		scaled_step *= 10.0;
		++decimals;
	}

	return std::max(decimals, 0);
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
	connect(&controller_, &ClothEditorController::selected_object_value_updated, this, [this](const QString& value_id) {
		static_cast<void>(value_id);
		refresh_property_values();
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
	info_layout->addRow("Type", selected_source_label_);
	info_layout->addRow("Details", selected_solver_label_);
	info_layout->addRow("Metrics", selected_topology_label_);
	layout->addWidget(info_group);

	QGroupBox* properties_group = new QGroupBox("Properties", inspector_panel_);
	properties_layout_ = new QVBoxLayout(properties_group);
	properties_layout_->setContentsMargins(0, 0, 0, 0);
	properties_panel_ = properties_group;
	layout->addWidget(properties_group);

	QGroupBox* actions_group = new QGroupBox("Actions", inspector_panel_);
	QVBoxLayout* actions_layout = new QVBoxLayout(actions_group);
	QPushButton* reset_cloth_button = new QPushButton("Reset", actions_group);
	QPushButton* delete_cloth_button = new QPushButton("Delete", actions_group);
	reset_cloth_button->setObjectName("resetClothButton");
	delete_cloth_button->setObjectName("deleteClothButton");
	actions_layout->addWidget(reset_cloth_button);
	actions_layout->addWidget(delete_cloth_button);
	layout->addWidget(actions_group);
	layout->addStretch(1);

	connect(reset_cloth_button, &QPushButton::clicked, this, [this]() {
		controller_.reset_selected_cloth();
	});
	connect(delete_cloth_button, &QPushButton::clicked, this, [this]() {
		controller_.delete_selected_cloth();
	});

	setWidget(inspector_panel_);
}

void ClothInspectorDock::refresh()
{
	const ClothEditorController::ObjectViewState* object = controller_.selected_object();
	inspector_panel_->setEnabled(object != nullptr);

	if (!object)
	{
		selected_name_label_->setText("-");
		selected_source_label_->setText("-");
		selected_solver_label_->setText("-");
		selected_topology_label_->setText("-");
		clear_property_editors();
		return;
	}

	selected_name_label_->setText(object->name);
	selected_source_label_->setText(object->type_label);
	selected_solver_label_->setText(object->detail_label.isEmpty() ? "-" : object->detail_label);
	selected_topology_label_->setText(object->metrics_label.isEmpty() ? "-" : object->metrics_label);

	if (QPushButton* reset_button = inspector_panel_->findChild<QPushButton*>("resetClothButton"))
	{
		reset_button->setEnabled(object->is_cloth);
	}
	if (QPushButton* delete_button = inspector_panel_->findChild<QPushButton*>("deleteClothButton"))
	{
		delete_button->setEnabled(object->is_cloth);
	}

	rebuild_property_editors();
}

void ClothInspectorDock::refresh_property_values()
{
	const ClothEditorController::ObjectViewState* object = controller_.selected_object();
	if (!object)
	{
		return;
	}

	for (const PropertyEditorBinding& binding : property_editors_)
	{
		const ClothEditorController::PropertyViewState* property =
			find_selected_property(*object, binding.source_object_id, binding.property_id);
		if (!property)
		{
			continue;
		}

		switch (binding.type)
		{
		case PropertyType::Bool:
		{
			bool checked = false;
			if (!binding.check_box || !convert_property_value(property->value, checked))
			{
				break;
			}

			const QSignalBlocker blocker(binding.check_box);
			binding.check_box->setChecked(checked);
			break;
		}
		case PropertyType::Int:
		{
			int value = 0;
			if (!binding.spin_box || !convert_property_value(property->value, value))
			{
				break;
			}

			const QSignalBlocker blocker(binding.spin_box);
			binding.spin_box->setValue(value);
			break;
		}
		case PropertyType::Float:
		{
			float value = 0.0f;
			if (!binding.double_spin_box || !convert_property_value(property->value, value))
			{
				break;
			}

			const QSignalBlocker blocker(binding.double_spin_box);
			binding.double_spin_box->setValue(static_cast<double>(value));
			break;
		}
		case PropertyType::Vec3:
		{
			glm::vec3 value(0.0f);
			if (!binding.vec3_x || !binding.vec3_y || !binding.vec3_z
				|| !convert_property_value(property->value, value))
			{
				break;
			}

			const QSignalBlocker block_x(binding.vec3_x);
			const QSignalBlocker block_y(binding.vec3_y);
			const QSignalBlocker block_z(binding.vec3_z);
			binding.vec3_x->setValue(static_cast<double>(value.x));
			binding.vec3_y->setValue(static_cast<double>(value.y));
			binding.vec3_z->setValue(static_cast<double>(value.z));
			break;
		}
		case PropertyType::String:
		{
			std::string value;
			if (!binding.line_edit || !convert_property_value(property->value, value))
			{
				break;
			}

			const QSignalBlocker blocker(binding.line_edit);
			binding.line_edit->setText(QString::fromStdString(value));
			break;
		}
		}
	}
}

void ClothInspectorDock::rebuild_property_editors()
{
	clear_property_editors();

	const ClothEditorController::ObjectViewState* object = controller_.selected_object();
	if (!object)
	{
		return;
	}

	QString current_section_title;
	QFormLayout* current_form_layout = nullptr;

	for (const ClothEditorController::PropertyViewState& property : object->properties)
	{
		const QString section_title = property_section_title(property);
		if (!current_form_layout || section_title != current_section_title)
		{
			current_section_title = section_title;

			QGroupBox* section_group = new QGroupBox(section_title, properties_panel_);
			current_form_layout = new QFormLayout(section_group);
			properties_layout_->addWidget(section_group);
		}

		PropertyEditorBinding binding;
		binding.source_object_id = property.source_object_id;
		binding.property_id = property.id;
		binding.type = property.type;

		switch (property.type)
		{
		case PropertyType::Bool:
		{
			QCheckBox* check_box = new QCheckBox(properties_panel_);
			check_box->setEnabled(property.editable);
			binding.editor = check_box;
			binding.check_box = check_box;
			connect(check_box, &QCheckBox::toggled, this, [this, source_object_id = property.source_object_id, property_id = property.id](bool checked) {
				update_selected_object_property(source_object_id, property_id, checked);
			});
			current_form_layout->addRow(property.label, check_box);
			break;
		}
		case PropertyType::Int:
		{
			QSpinBox* spin_box = new QSpinBox(properties_panel_);
			spin_box->setRange(property_int_or(property.min_value, -1000000), property_int_or(property.max_value, 1000000));
			spin_box->setSingleStep(property_int_or(property.step, 1));
			spin_box->setKeyboardTracking(false);
			spin_box->setEnabled(property.editable);
			binding.editor = spin_box;
			binding.spin_box = spin_box;
			connect(spin_box, qOverload<int>(&QSpinBox::valueChanged), this, [this, source_object_id = property.source_object_id, property_id = property.id](int value) {
				update_selected_object_property(source_object_id, property_id, value);
			});
			current_form_layout->addRow(property.label, spin_box);
			break;
		}
		case PropertyType::Float:
		{
			QDoubleSpinBox* spin_box = new QDoubleSpinBox(properties_panel_);
			configure_double_spin_box(
				spin_box,
				property_double_or(property.min_value, -1000000.0),
				property_double_or(property.max_value, 1000000.0),
				property_double_or(property.step, 0.01),
				decimals_for_step(property.step));
			spin_box->setEnabled(property.editable);
			binding.editor = spin_box;
			binding.double_spin_box = spin_box;
			connect(spin_box, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, source_object_id = property.source_object_id, property_id = property.id](double value) {
				update_selected_object_property(source_object_id, property_id, static_cast<float>(value));
			});
			current_form_layout->addRow(property.label, spin_box);
			break;
		}
		case PropertyType::Vec3:
		{
			QWidget* row_widget = new QWidget(properties_panel_);
			QHBoxLayout* row_layout = new QHBoxLayout(row_widget);
			row_layout->setContentsMargins(0, 0, 0, 0);
			row_layout->setSpacing(4);

			QDoubleSpinBox* x_spin = new QDoubleSpinBox(row_widget);
			QDoubleSpinBox* y_spin = new QDoubleSpinBox(row_widget);
			QDoubleSpinBox* z_spin = new QDoubleSpinBox(row_widget);
			configure_double_spin_box(x_spin, property_double_or(property.min_value, -1000000.0), property_double_or(property.max_value, 1000000.0), property_double_or(property.step, 0.01), decimals_for_step(property.step));
			configure_double_spin_box(y_spin, property_double_or(property.min_value, -1000000.0), property_double_or(property.max_value, 1000000.0), property_double_or(property.step, 0.01), decimals_for_step(property.step));
			configure_double_spin_box(z_spin, property_double_or(property.min_value, -1000000.0), property_double_or(property.max_value, 1000000.0), property_double_or(property.step, 0.01), decimals_for_step(property.step));
			x_spin->setEnabled(property.editable);
			y_spin->setEnabled(property.editable);
			z_spin->setEnabled(property.editable);
			row_layout->addWidget(x_spin);
			row_layout->addWidget(y_spin);
			row_layout->addWidget(z_spin);

			binding.editor = row_widget;
			binding.vec3_x = x_spin;
			binding.vec3_y = y_spin;
			binding.vec3_z = z_spin;

			const auto commit_vec3 = [this, source_object_id = property.source_object_id, property_id = property.id, x_spin, y_spin, z_spin]() {
				update_selected_object_property(
					source_object_id,
					property_id,
					glm::vec3(
						static_cast<float>(x_spin->value()),
						static_cast<float>(y_spin->value()),
						static_cast<float>(z_spin->value())));
			};

			connect(x_spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [commit_vec3](double) { commit_vec3(); });
			connect(y_spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [commit_vec3](double) { commit_vec3(); });
			connect(z_spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [commit_vec3](double) { commit_vec3(); });
			current_form_layout->addRow(property.label, row_widget);
			break;
		}
		case PropertyType::String:
		{
			QLineEdit* line_edit = new QLineEdit(properties_panel_);
			line_edit->setEnabled(property.editable);
			binding.editor = line_edit;
			binding.line_edit = line_edit;
			connect(line_edit, &QLineEdit::editingFinished, this, [this, source_object_id = property.source_object_id, property_id = property.id, line_edit]() {
				update_selected_object_property(source_object_id, property_id, line_edit->text().toStdString());
			});
			current_form_layout->addRow(property.label, line_edit);
			break;
		}
		}

		property_editors_.push_back(binding);
	}

	properties_layout_->addStretch(1);
	refresh_property_values();
}

void ClothInspectorDock::clear_property_editors()
{
	property_editors_.clear();

	if (!properties_layout_)
	{
		return;
	}

	while (QLayoutItem* item = properties_layout_->takeAt(0))
	{
		if (QWidget* widget = item->widget())
		{
			widget->deleteLater();
		}

		delete item;
	}
}

void ClothInspectorDock::update_selected_object_property(
	std::uint64_t source_object_id,
	const QString& property_id,
	const PropertyValue& value)
{
	controller_.update_selected_object_property(source_object_id, property_id, value);
}
