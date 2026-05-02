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

ClothEditorController::PropertyViewState make_property_view_state(std::uint64_t source_object_id, const PropertyBase& property)
{
	ClothEditorController::PropertyViewState property_view;
	property_view.source_object_id = source_object_id;
	property_view.id = QString::fromStdString(property.descriptor().id);
	property_view.label = QString::fromStdString(property.descriptor().label);
	property_view.group = QString::fromStdString(property.descriptor().group);
	property_view.type = property.descriptor().type;
	property_view.editable = property.descriptor().editable;
	property_view.value = property.value();
	return property_view;
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

void ClothEditorController::spawn_sphere_from_view()
{
	engine_.enqueue_world_job([](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->spawn_sphere_projectile();
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

void ClothEditorController::update_selected_cloth_property(
	std::uint64_t source_object_id,
	const QString& property_id,
	const PropertyValue& value)
{
	const ClothViewState* cloth = selected_cloth();
	if (!cloth)
	{
		return;
	}

	engine_.enqueue_world_job([
		cloth_id = cloth->id,
		source_object_id,
		property_id = property_id.toStdString(),
		value](World& world) {
		if (ClothWorld* cloth_world = dynamic_cast<ClothWorld*>(&world))
		{
			cloth_world->set_cloth_property(cloth_id, source_object_id, std::move(property_id), value);
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
			cloth_world->set_change_callback([this](const ClothWorld::ChangeEvent& change) {
				QMetaObject::invokeMethod(this, [this, change]() {
					handle_world_change(change);
				}, Qt::QueuedConnection);
			});
		}
	});
}

void ClothEditorController::handle_world_change(const ClothWorld::ChangeEvent& change)
{
	switch (change.kind)
	{
	case ClothWorld::ChangeEvent::Kind::SnapshotInvalidated:
		request_snapshot();
		return;
	case ClothWorld::ChangeEvent::Kind::SelectionChanged:
		snapshot_.selected_cloth_id = change.selected_cloth_id;
		emit snapshot_updated();
		return;
	case ClothWorld::ChangeEvent::Kind::ClothValueChanged:
		if (!apply_cloth_value_change(change))
		{
			request_snapshot();
			return;
		}

		if (change.cloth_id == snapshot_.selected_cloth_id)
		{
			emit selected_cloth_value_updated(QString::fromStdString(change.value_id));
		}
		return;
	}
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
			cloth_view.id = cloth_object->id();
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
				cloth_view.properties.reserve(simulation_component->properties().size());
				for (const PropertyBase* property : simulation_component->properties())
				{
					if (!property)
					{
						continue;
					}

					cloth_view.properties.push_back(make_property_view_state(simulation_component->id(), *property));
				}

				if (const std::optional<float> damping = simulation_component->get_typed_property<float>("damping"))
				{
					cloth_view.damping = *damping;
				}

				if (const std::optional<int> constraint_iterations =
					simulation_component->get_typed_property<int>("constraint_iterations"))
				{
					cloth_view.constraint_iterations = *constraint_iterations;
				}
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

ClothEditorController::ClothViewState* ClothEditorController::find_cloth_view(std::uint64_t cloth_id)
{
	for (ClothViewState& cloth : snapshot_.cloths)
	{
		if (cloth.id == cloth_id)
		{
			return &cloth;
		}
	}

	return nullptr;
}

bool ClothEditorController::apply_cloth_value_change(const ClothWorld::ChangeEvent& change)
{
	ClothViewState* cloth = find_cloth_view(change.cloth_id);
	if (!cloth || !change.value)
	{
		return false;
	}

	if (change.value_id == "position")
	{
		glm::vec3 position;
		if (!convert_property_value(*change.value, position))
		{
			return false;
		}

		cloth->position_x = position.x;
		cloth->position_y = position.y;
		cloth->position_z = position.z;
		return true;
	}

	if (change.value_id == "anchors_enabled")
	{
		bool anchors_enabled = false;
		if (!convert_property_value(*change.value, anchors_enabled))
		{
			return false;
		}

		cloth->anchors_enabled = anchors_enabled;
		return true;
	}

	for (PropertyViewState& property : cloth->properties)
	{
		if (property.source_object_id == change.source_object_id
			&& property.id == QString::fromStdString(change.value_id))
		{
			property.value = *change.value;

			if (change.value_id == "damping")
			{
				float damping = 0.0f;
				if (!convert_property_value(*change.value, damping))
				{
					return false;
				}

				cloth->damping = damping;
			}
			else if (change.value_id == "constraint_iterations")
			{
				int constraint_iterations = 0;
				if (!convert_property_value(*change.value, constraint_iterations))
				{
					return false;
				}

				cloth->constraint_iterations = constraint_iterations;
			}

			return true;
		}
	}

	return false;
}
