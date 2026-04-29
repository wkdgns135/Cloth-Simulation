#include "platform/qt/ClothEditorController.h"

#include <exception>
#include <utility>

#include <QMessageBox>
#include <QMetaObject>
#include <QWidget>

#include "cloth/ClothWorld.h"
#include "cloth/components/ClothSimulationComponentBase.h"
#include "engine/Engine.h"

namespace
{
QString solver_display_name(ClothSolverKind solver_kind)
{
	switch (solver_kind)
	{
	case ClothSolverKind::XPBD:
		return "XPBD";
	case ClothSolverKind::PBD:
	default:
		return "PBD";
	}
}

QString source_display_name(ClothSourceKind source_kind)
{
	switch (source_kind)
	{
	case ClothSourceKind::Mesh:
		return "Mesh";
	case ClothSourceKind::Grid:
	default:
		return "Grid";
	}
}
}

ClothEditorController::ClothEditorController(Engine& engine, QObject* parent)
	: QObject(parent)
	, engine_(engine)
{
	bind_world_notifications();
	request_snapshot();
}

ClothEditorController::~ClothEditorController()
{
	engine_.enqueue_world_job([](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->set_change_callback({});
		}
	});
}

const ClothEditorController::ClothViewState* ClothEditorController::selected_cloth() const
{
	for (const ClothViewState& cloth : snapshot_.cloths)
	{
		if (cloth.id == snapshot_.selected_cloth_id)
		{
			return &cloth;
		}
	}

	return nullptr;
}

void ClothEditorController::create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind)
{
	engine_.enqueue_world_job([width, height, spacing, solver_kind](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->create_grid_cloth(width, height, spacing, solver_kind);
		}
	});
}

void ClothEditorController::create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind)
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

void ClothEditorController::set_selected_cloth(std::uint64_t cloth_id)
{
	engine_.enqueue_world_job([cloth_id](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->select_cloth(cloth_id);
		}
	});
}

void ClothEditorController::update_selected_cloth_position(float x, float y, float z)
{
	const ClothViewState* cloth = selected_cloth();
	if (!cloth)
	{
		return;
	}

	const glm::vec3 position(x, y, z);
	engine_.enqueue_world_job([cloth_id = cloth->id, position](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->set_cloth_position(cloth_id, position);
		}
	});
}

void ClothEditorController::update_selected_cloth_simulation_settings(float damping, int constraint_iterations)
{
	const ClothViewState* cloth = selected_cloth();
	if (!cloth)
	{
		return;
	}

	engine_.enqueue_world_job([cloth_id = cloth->id, damping, constraint_iterations](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->set_cloth_simulation_settings(cloth_id, damping, constraint_iterations);
		}
	});
}

void ClothEditorController::reset_selected_cloth()
{
	const ClothViewState* cloth = selected_cloth();
	if (!cloth)
	{
		return;
	}

	engine_.enqueue_world_job([cloth_id = cloth->id](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->reset_cloth(cloth_id);
		}
	});
}

void ClothEditorController::toggle_selected_cloth_anchors()
{
	const ClothViewState* cloth = selected_cloth();
	if (!cloth)
	{
		return;
	}

	engine_.enqueue_world_job([cloth_id = cloth->id](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->toggle_cloth_anchors(cloth_id);
		}
	});
}

void ClothEditorController::delete_selected_cloth()
{
	const ClothViewState* cloth = selected_cloth();
	if (!cloth)
	{
		return;
	}

	engine_.enqueue_world_job([cloth_id = cloth->id](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->destroy_cloth(cloth_id);
		}
	});
}

void ClothEditorController::bind_world_notifications()
{
	engine_.enqueue_world_job([this](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->set_change_callback([this]() {
				QMetaObject::invokeMethod(this, [this]() {
					request_snapshot();
				}, Qt::QueuedConnection);
			});
		}
	});
}

void ClothEditorController::request_snapshot()
{
	if (snapshot_request_pending_)
	{
		return;
	}

	snapshot_request_pending_ = true;
	engine_.enqueue_world_job([this](World& world) {
		WorldViewState snapshot;

		if (const ClothWorld* cloth_world = dynamic_cast<const ClothWorld*>(&world))
		{
			snapshot.selected_cloth_id = cloth_world->selected_cloth_id();
		}

		const World& current_world = world;
		for (const ClothObject* cloth_object : current_world.get_objects<ClothObject>())
		{
			const Cloth& cloth = cloth_object->cloth();
			ClothViewState cloth_view;
			cloth_view.id = cloth_object->cloth_id();
			cloth_view.name = QString::fromStdString(cloth_object->display_name());
			cloth_view.source_label = QString::fromStdString(cloth_object->source_label());
			cloth_view.source_kind_label = source_display_name(cloth_object->source_kind());
			cloth_view.solver_label = solver_display_name(cloth_object->solver_kind());
			cloth_view.particle_count = static_cast<int>(cloth.get_particles().size());
			cloth_view.triangle_count = static_cast<int>(cloth.get_indices().size() / 3);
			cloth_view.anchors_enabled = cloth_object->anchors_enabled();
			cloth_view.position_x = cloth_object->transform().position.x;
			cloth_view.position_y = cloth_object->transform().position.y;
			cloth_view.position_z = cloth_object->transform().position.z;

			if (const ClothSimulationComponentBase* simulation_component = cloth_object->simulation_component())
			{
				cloth_view.damping = simulation_component->damping();
				cloth_view.constraint_iterations = simulation_component->constraint_iterations();
			}

			snapshot.cloths.push_back(std::move(cloth_view));
		}

		QMetaObject::invokeMethod(this, [this, snapshot = std::move(snapshot)]() mutable {
			snapshot_request_pending_ = false;
			apply_snapshot(std::move(snapshot));
		}, Qt::QueuedConnection);
	});
}

void ClothEditorController::apply_snapshot(WorldViewState snapshot)
{
	snapshot_ = std::move(snapshot);
	emit snapshot_updated();
}
