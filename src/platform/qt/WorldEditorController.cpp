#include "platform/qt/WorldEditorController.h"

#include <string_view>

#include <QMetaObject>

#include "cloth/ClothObject.h"
#include "engine/objects/CameraObject.h"
#include "engine/objects/DirectionalLightObject.h"
#include "engine/objects/PlaneObject.h"
#include "engine/objects/SphereObject.h"
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

QString object_type_display_name(const WorldObject& object)
{
	if (dynamic_cast<const ClothObject*>(&object))
	{
		return "Cloth";
	}
	if (dynamic_cast<const CameraObject*>(&object))
	{
		return "Camera";
	}
	if (dynamic_cast<const DirectionalLightObject*>(&object))
	{
		return "Directional Light";
	}
	if (dynamic_cast<const PlaneObject*>(&object))
	{
		return "Plane";
	}
	if (dynamic_cast<const SphereObject*>(&object))
	{
		return "Sphere";
	}

	return "Object";
}

QString object_detail_display_name(const WorldObject& object)
{
	if (const auto* cloth_object = dynamic_cast<const ClothObject*>(&object))
	{
		return QString("%1  |  %2  |  %3")
			.arg(source_display_name(cloth_object->source_kind()))
			.arg(QString::fromStdString(cloth_object->source_label()))
			.arg(solver_display_name(cloth_object->solver_kind()));
	}

	return QString();
}

QString object_metrics_display_name(const WorldObject& object)
{
	if (const auto* cloth_object = dynamic_cast<const ClothObject*>(&object))
	{
		const Cloth& cloth = cloth_object->cloth();
		return QString("%1 particles / %2 triangles")
			.arg(static_cast<int>(cloth.get_particles().size()))
			.arg(static_cast<int>(cloth.get_indices().size() / 3));
	}

	return QString("%1 component(s)").arg(static_cast<int>(object.components().size()));
}

WorldEditorController::PropertyViewState make_property_view_state(
	std::uint64_t source_object_id,
	const QString& source_label,
	const PropertyBase& property)
{
	WorldEditorController::PropertyViewState property_view;
	property_view.source_object_id = source_object_id;
	property_view.source_label = source_label;
	property_view.id = QString::fromStdString(property.descriptor().id);
	property_view.label = QString::fromStdString(property.descriptor().label);
	property_view.group = QString::fromStdString(property.descriptor().group);
	property_view.type = property.descriptor().type;
	property_view.editable = property.descriptor().editable;
	property_view.value = property.value();
	property_view.min_value = property.descriptor().min_value;
	property_view.max_value = property.descriptor().max_value;
	property_view.step = property.descriptor().step;
	return property_view;
}

}

WorldEditorController::WorldEditorController(Engine& engine, QObject* parent)
	: QObject(parent)
	, engine_(engine)
{
	bind_world_notifications();
	request_snapshot();
}

WorldEditorController::~WorldEditorController()
{
	engine_.enqueue_world_job([](World& world) {
		world.set_change_callback({});
	});
}

const WorldEditorController::ObjectViewState* WorldEditorController::selected_object() const
{
	for (const ObjectViewState& object : snapshot_.objects)
	{
		if (object.id == snapshot_.selected_object_id)
		{
			return &object;
		}
	}

	return nullptr;
}

void WorldEditorController::set_selected_object(std::uint64_t object_id)
{
	engine_.enqueue_world_job([object_id](World& world) {
		world.select_object(object_id);
	});
}

void WorldEditorController::update_selected_object_property(
	std::uint64_t source_object_id,
	const QString& property_id,
	const PropertyValue& value)
{
	const ObjectViewState* object = selected_object();
	if (!object)
	{
		return;
	}

	engine_.enqueue_world_job([
		source_object_id,
		property_id = property_id.toStdString(),
		value](World& world) {
		world.set_runtime_object_property(source_object_id, std::move(property_id), value);
	});
}

void WorldEditorController::bind_world_notifications()
{
	engine_.enqueue_world_job([this](World& world) {
		world.set_change_callback([this](const World::ChangeEvent& change) {
			QMetaObject::invokeMethod(this, [this, change]() {
				handle_world_change(change);
			}, Qt::QueuedConnection);
		});
	});
}

void WorldEditorController::handle_world_change(const World::ChangeEvent& change)
{
	switch (change.kind)
	{
	case World::ChangeEvent::Kind::SnapshotInvalidated:
		request_snapshot();
		return;
	case World::ChangeEvent::Kind::SelectionChanged:
		snapshot_.selected_object_id = change.selected_object_id;
		emit snapshot_updated();
		return;
	case World::ChangeEvent::Kind::ObjectValueChanged:
		if (!apply_object_value_change(change))
		{
			request_snapshot();
			return;
		}

		if (change.object_id == snapshot_.selected_object_id)
		{
			emit selected_object_value_updated(QString::fromStdString(change.value_id));
		}
		return;
	}
}

void WorldEditorController::request_snapshot()
{
	if (snapshot_request_pending_)
	{
		snapshot_refresh_requested_ = true;
		return;
	}

	snapshot_request_pending_ = true;
	engine_.enqueue_world_job([this](World& world) {
		WorldViewState snapshot;
		snapshot.selected_object_id = world.selected_object_id();

		const World& current_world = world;
		for (const WorldObject* world_object : current_world.get_objects<WorldObject>())
		{
			ObjectViewState object_view;
			object_view.id = world_object->id();
			object_view.name = QString::fromStdString(world_object->display_name());
			object_view.type_label = object_type_display_name(*world_object);
			object_view.detail_label = object_detail_display_name(*world_object);
			object_view.metrics_label = object_metrics_display_name(*world_object);
			object_view.is_cloth = dynamic_cast<const ClothObject*>(world_object) != nullptr;

			if (!world_object->properties().empty())
			{
				for (const PropertyBase* property : world_object->properties())
				{
					if (!property)
					{
						continue;
					}

					object_view.properties.push_back(make_property_view_state(
						world_object->id(),
						object_view.type_label,
						*property));
				}
			}

			for (const std::unique_ptr<Component>& component : world_object->components())
			{
				if (!component || component->properties().empty())
				{
					continue;
				}

				const QString source_label =
					component->display_name().empty()
					? QStringLiteral("Component")
					: QString::fromStdString(component->display_name());

				for (const PropertyBase* property : component->properties())
				{
					if (!property)
					{
						continue;
					}

					object_view.properties.push_back(make_property_view_state(
						component->id(),
						source_label,
						*property));
				}
			}

			snapshot.objects.push_back(std::move(object_view));
		}

		QMetaObject::invokeMethod(this, [this, snapshot = std::move(snapshot)]() mutable {
			snapshot_request_pending_ = false;
			apply_snapshot(std::move(snapshot));
		}, Qt::QueuedConnection);
	});
}

void WorldEditorController::apply_snapshot(WorldViewState snapshot)
{
	snapshot_ = std::move(snapshot);
	emit snapshot_updated();

	if (snapshot_refresh_requested_)
	{
		snapshot_refresh_requested_ = false;
		request_snapshot();
	}
}

WorldEditorController::ObjectViewState* WorldEditorController::find_object_view(std::uint64_t object_id)
{
	for (ObjectViewState& object : snapshot_.objects)
	{
		if (object.id == object_id)
		{
			return &object;
		}
	}

	return nullptr;
}

bool WorldEditorController::apply_object_value_change(const World::ChangeEvent& change)
{
	ObjectViewState* object = find_object_view(change.object_id);
	if (!object || !change.value)
	{
		return false;
	}

	bool property_updated = false;
	for (PropertyViewState& property : object->properties)
	{
		if (property.source_object_id == change.source_object_id
			&& property.id == QString::fromStdString(change.value_id))
		{
			property.value = *change.value;
			property_updated = true;
			break;
		}
	}

	return property_updated;
}
