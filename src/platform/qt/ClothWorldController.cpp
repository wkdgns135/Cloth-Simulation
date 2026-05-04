#include "platform/qt/ClothWorldController.h"

#include <exception>

#include <QMetaObject>
#include <QMessageBox>
#include <QWidget>

#include "cloth/ClothWorld.h"
#include "engine/Engine.h"

ClothWorldController::ClothWorldController(Engine& engine, QObject* parent)
	: QObject(parent)
	, engine_(engine)
{
}

void ClothWorldController::create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind)
{
	engine_.enqueue_world_job([width, height, spacing, solver_kind](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->create_grid_cloth(width, height, spacing, solver_kind);
		}
	});
}

void ClothWorldController::create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind)
{
	engine_.enqueue_world_job([this, mesh_path, solver_kind](World& world) {
		try
		{
			if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
			{
				cloth_world->create_mesh_cloth(mesh_path, solver_kind);
			}
		}
		catch (const std::exception& exception)
		{
			const QString message = QString::fromStdString(exception.what());
			QMetaObject::invokeMethod(this, [this, message]() {
				QMessageBox::critical(qobject_cast<QWidget*>(parent()), "Failed to Add Cloth", message);
			}, Qt::QueuedConnection);
		}
	});
}

void ClothWorldController::spawn_sphere_from_view()
{
	engine_.enqueue_world_job([](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->spawn_sphere_projectile();
		}
	});
}

void ClothWorldController::reset_selected_cloth()
{
	engine_.enqueue_world_job([](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->reset_selected_cloth();
		}
	});
}

void ClothWorldController::toggle_selected_cloth_anchors()
{
	engine_.enqueue_world_job([](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->toggle_selected_cloth_anchors();
		}
	});
}

void ClothWorldController::delete_selected_cloth()
{
	engine_.enqueue_world_job([](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->delete_selected_cloth();
		}
	});
}
