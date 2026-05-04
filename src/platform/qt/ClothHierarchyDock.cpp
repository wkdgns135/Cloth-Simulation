#include "platform/qt/ClothHierarchyDock.h"

#include <filesystem>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVariant>
#include <QVBoxLayout>

#include "platform/qt/ClothEditorController.h"

namespace
{
QString object_list_entry_text(const ClothEditorController::ObjectViewState& object)
{
	if (!object.metrics_label.isEmpty())
	{
		return QString("%1 [%2]  %3")
			.arg(object.name)
			.arg(object.type_label)
			.arg(object.metrics_label);
	}

	return QString("%1 [%2]")
		.arg(object.name)
		.arg(object.type_label);
}

ClothSolverKind combo_box_solver_kind(const QComboBox* combo_box)
{
	return static_cast<ClothSolverKind>(combo_box->currentData().toInt());
}

void configure_double_spin_box(QDoubleSpinBox* spin_box, double minimum, double maximum, double step, int decimals)
{
	spin_box->setRange(minimum, maximum);
	spin_box->setSingleStep(step);
	spin_box->setDecimals(decimals);
	spin_box->setKeyboardTracking(false);
}
}

ClothHierarchyDock::ClothHierarchyDock(ClothEditorController& controller, QWidget* parent)
	: QDockWidget("Hierarchy", parent)
	, controller_(controller)
{
	setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	build_ui();

	connect(&controller_, &ClothEditorController::snapshot_updated, this, [this]() {
		refresh();
	});

	refresh();
}

void ClothHierarchyDock::build_ui()
{
	QWidget* contents = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(contents);
	layout->setContentsMargins(8, 8, 8, 8);

	QGroupBox* create_group = new QGroupBox("Create", contents);
	QFormLayout* create_layout = new QFormLayout(create_group);

	new_solver_combo_ = new QComboBox(create_group);
	new_solver_combo_->addItem("PBD", static_cast<int>(ClothSolverKind::PBD));
	new_solver_combo_->addItem("XPBD", static_cast<int>(ClothSolverKind::XPBD));
	create_layout->addRow("Solver", new_solver_combo_);

	new_grid_width_spin_ = new QSpinBox(create_group);
	new_grid_width_spin_->setRange(2, 512);
	new_grid_width_spin_->setValue(32);
	create_layout->addRow("Grid Width", new_grid_width_spin_);

	new_grid_height_spin_ = new QSpinBox(create_group);
	new_grid_height_spin_->setRange(2, 512);
	new_grid_height_spin_->setValue(32);
	create_layout->addRow("Grid Height", new_grid_height_spin_);

	new_grid_spacing_spin_ = new QDoubleSpinBox(create_group);
	configure_double_spin_box(new_grid_spacing_spin_, 0.001, 10.0, 0.01, 3);
	new_grid_spacing_spin_->setValue(0.05);
	create_layout->addRow("Spacing", new_grid_spacing_spin_);

	QHBoxLayout* create_buttons_layout = new QHBoxLayout();
	QPushButton* add_grid_button = new QPushButton("Add Grid", create_group);
	QPushButton* add_mesh_button = new QPushButton("Add Mesh...", create_group);
	create_buttons_layout->addWidget(add_grid_button);
	create_buttons_layout->addWidget(add_mesh_button);
	create_layout->addRow(create_buttons_layout);

	QPushButton* spawn_sphere_button = new QPushButton("Spawn Sphere", create_group);
	create_layout->addRow(spawn_sphere_button);

	connect(add_grid_button, &QPushButton::clicked, this, [this]() {
		create_grid_cloth();
	});
	connect(add_mesh_button, &QPushButton::clicked, this, [this]() {
		create_mesh_cloth();
	});
	connect(spawn_sphere_button, &QPushButton::clicked, this, [this]() {
		spawn_sphere();
	});

	layout->addWidget(create_group);

	object_list_widget_ = new QListWidget(contents);
	object_list_widget_->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(object_list_widget_, &QListWidget::itemSelectionChanged, this, [this]() {
		const QList<QListWidgetItem*> selected_items = object_list_widget_->selectedItems();
		if (selected_items.isEmpty())
		{
			controller_.set_selected_object(0);
			return;
		}

		controller_.set_selected_object(selected_items.front()->data(Qt::UserRole).toULongLong());
	});

	layout->addWidget(object_list_widget_, 1);
	setWidget(contents);
}

void ClothHierarchyDock::refresh()
{
	QSignalBlocker blocker(object_list_widget_);
	object_list_widget_->clear();

	const ClothEditorController::WorldViewState& snapshot = controller_.snapshot();
	QListWidgetItem* selected_item = nullptr;

	for (const ClothEditorController::ObjectViewState& object : snapshot.objects)
	{
		QListWidgetItem* item = new QListWidgetItem(object_list_entry_text(object), object_list_widget_);
		item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(static_cast<qulonglong>(object.id)));
		QString tooltip = object.name;
		if (!object.detail_label.isEmpty())
		{
			tooltip += "\n" + object.detail_label;
		}
		item->setToolTip(tooltip);

		if (object.id == snapshot.selected_object_id)
		{
			selected_item = item;
		}
	}

	if (selected_item)
	{
		object_list_widget_->setCurrentItem(selected_item);
	}
}

void ClothHierarchyDock::create_grid_cloth()
{
	controller_.create_grid_cloth(
		new_grid_width_spin_->value(),
		new_grid_height_spin_->value(),
		static_cast<float>(new_grid_spacing_spin_->value()),
		combo_box_solver_kind(new_solver_combo_));
}

void ClothHierarchyDock::create_mesh_cloth()
{
	const QString file_path = QFileDialog::getOpenFileName(
		this,
		"Add Mesh Cloth",
		QString(),
		"Wavefront OBJ (*.obj);;All Files (*)");
	if (file_path.isEmpty())
	{
		return;
	}

	controller_.create_mesh_cloth(file_path.toStdString(), combo_box_solver_kind(new_solver_combo_));
}

void ClothHierarchyDock::spawn_sphere()
{
	controller_.spawn_sphere_from_view();
}
