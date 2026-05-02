#include "cloth/ClothWorld.h"

#include <algorithm>
#include <limits>

#include <glm/geometric.hpp>

#include "cloth/components/ClothCameraInputComponent.h"
#include "cloth/components/ClothSimulationComponentBase.h"
#include "cloth/components/PBDClothSimulationComponent.h"
#include "cloth/components/XPBDClothSimulationComponent.h"
#include "engine/objects/CameraObject.h"
#include "engine/objects/DirectionalLightObject.h"
#include "engine/objects/PlaneObject.h"
#include "engine/objects/SphereObject.h"

namespace
{
constexpr float kDefaultCollisionPlaneClearance = 0.1f;
constexpr float kSpawnSphereRadius = 0.5f;
constexpr float kSpawnSphereSpeed = 3.5f;
constexpr float kSpawnSphereForwardOffset = 0.45f;
constexpr float kSpawnSphereMaxTravelDistance = 12.0f;

float compute_default_floor_height(const ClothObject& cloth_object)
{
	const std::vector<Particle>& particles = cloth_object.cloth().get_particles();
	if (particles.empty())
	{
		return -1.0f;
	}

	const glm::mat4 object_transform = cloth_object.transform().matrix();
	float min_y = std::numeric_limits<float>::max();

	for (const Particle& particle : particles)
	{
		const glm::vec3 world_position = glm::vec3(object_transform * glm::vec4(particle.position, 1.0f));
		min_y = std::min(min_y, world_position.y);
	}

	return min_y - kDefaultCollisionPlaneClearance;
}
}

ClothWorld::ClothWorld()
{
	main_camera_object_->transform().position = glm::vec3(0.0f, 0.0f, 2.6f);
	main_camera_object_->transform().look_at(glm::vec3(0.0f, 0.0f, 0.0f));
	main_camera_object_->fov_y_degrees = 45.0f;
	main_camera_object_->add_component<ClothCameraInputComponent>();

	main_directional_light_object_->transform().set_forward(glm::normalize(glm::vec3(-0.75f, 0.65f, 0.45f)));
	main_directional_light_object_->ambient_strength = 0.16f;
	main_directional_light_object_->diffuse_strength = 1.15f;

	ClothObject& initial_cloth = create_mesh_cloth("asset/test_cloth_patch.obj", ClothSolverKind::PBD);
	initial_cloth.transform().position = glm::vec3(0.0f, 0.0f, 0.0f);

	PlaneObject& floor_object = create_object<PlaneObject>();
	floor_object.transform().position = glm::vec3(0.0f, compute_default_floor_height(initial_cloth), 0.0f);
	floor_object.transform().scale = glm::vec3(8.0f, 1.0f, 8.0f);
}

ClothObject& ClothWorld::create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind)
{
	const int existing_cloth_count = cloth_count();
	ClothObject& cloth_object = create_object<ClothObject>(
		std::string(),
		width,
		height,
		spacing);
	cloth_object.set_display_name(make_default_cloth_name(cloth_object.id(), solver_kind));
	attach_solver(cloth_object, solver_kind);
	cloth_object.transform().position = glm::vec3(0.35f * static_cast<float>(existing_cloth_count), 0.0f, 0.0f);
	selected_cloth_id_ = cloth_object.id();
	notify_snapshot_invalidated();
	return cloth_object;
}

PlaneObject& ClothWorld::create_plane_object(const glm::vec3& position, const glm::vec3& scale)
{
	PlaneObject& plane_object = create_object<PlaneObject>();
	plane_object.transform().position = position;
	plane_object.transform().scale = scale;
	notify_snapshot_invalidated();
	return plane_object;
}

SphereObject& ClothWorld::create_sphere_object(float radius, const glm::vec3& position)
{
	SphereObject& sphere_object = create_object<SphereObject>(radius);
	sphere_object.transform().position = position;
	notify_snapshot_invalidated();
	return sphere_object;
}

void ClothWorld::spawn_sphere_projectile()
{
	if (!main_camera_object_)
	{
		return;
	}

	const glm::vec3 forward = main_camera_object_->transform().forward();
	const glm::vec3 spawn_position = main_camera_object_->transform().position + forward * kSpawnSphereForwardOffset;

	SphereObject& sphere_object = create_object<SphereObject>(kSpawnSphereRadius);
	sphere_object.transform().position = spawn_position;
	sphere_object.configure_projectile(forward * kSpawnSphereSpeed, kSpawnSphereMaxTravelDistance);
	notify_snapshot_invalidated();
}

ClothObject& ClothWorld::create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind)
{
	const int existing_cloth_count = cloth_count();
	ClothObject& cloth_object = create_object<ClothObject>(
		std::string(),
		mesh_path);
	cloth_object.set_display_name(make_default_cloth_name(cloth_object.id(), solver_kind));
	attach_solver(cloth_object, solver_kind);
	cloth_object.transform().position = glm::vec3(0.35f * static_cast<float>(existing_cloth_count), 0.0f, 0.0f);
	selected_cloth_id_ = cloth_object.id();
	notify_snapshot_invalidated();
	return cloth_object;
}

bool ClothWorld::destroy_cloth(ClothId cloth_id)
{
	ClothObject* cloth_object = find_cloth(cloth_id);
	if (!cloth_object)
	{
		return false;
	}

	const bool removed = destroy_object(cloth_object);
	if (!removed)
	{
		return false;
	}

	if (selected_cloth_id_ == cloth_id)
	{
		selected_cloth_id_ = 0;

		for (const ClothObject* remaining_cloth : get_objects<ClothObject>())
		{
			selected_cloth_id_ = remaining_cloth->cloth_id();
			break;
		}
	}

	notify_snapshot_invalidated();
	return true;
}

bool ClothWorld::select_cloth(ClothId cloth_id)
{
	if (cloth_id == 0)
	{
		if (selected_cloth_id_ == 0)
		{
			return true;
		}

		selected_cloth_id_ = 0;
		notify_selection_changed();
		return true;
	}

	if (!find_cloth(cloth_id))
	{
		return false;
	}

	if (selected_cloth_id_ == cloth_id)
	{
		return true;
	}

	selected_cloth_id_ = cloth_id;
	notify_selection_changed();
	return true;
}

bool ClothWorld::set_cloth_position(ClothId cloth_id, const glm::vec3& position)
{
	ClothObject* cloth_object = find_cloth(cloth_id);
	if (!cloth_object)
	{
		return false;
	}

	cloth_object->transform().position = position;
	notify_cloth_value_changed(cloth_id, cloth_id, "position", position);
	return true;
}

bool ClothWorld::set_cloth_property(
	ClothId cloth_id,
	ObjectId source_object_id,
	std::string property_id,
	const PropertyValue& value)
{
	ClothObject* cloth_object = find_cloth(cloth_id);
	if (!cloth_object)
	{
		return false;
	}

	Object* target_object = nullptr;
	if (source_object_id == cloth_object->id())
	{
		target_object = cloth_object;
	}
	else
	{
		target_object = cloth_object->find_component<Component>(source_object_id);
	}

	if (!target_object)
	{
		return false;
	}

	return target_object->set_property(property_id, value);
}

bool ClothWorld::reset_cloth(ClothId cloth_id)
{
	ClothObject* cloth_object = find_cloth(cloth_id);
	if (!cloth_object)
	{
		return false;
	}

	cloth_object->reset_to_initial_state();
	notify_cloth_value_changed(cloth_id, cloth_id, "anchors_enabled", cloth_object->anchors_enabled());
	return true;
}

bool ClothWorld::toggle_cloth_anchors(ClothId cloth_id)
{
	ClothObject* cloth_object = find_cloth(cloth_id);
	if (!cloth_object)
	{
		return false;
	}

	cloth_object->toggle_anchor_state();
	notify_cloth_value_changed(cloth_id, cloth_id, "anchors_enabled", cloth_object->anchors_enabled());
	return true;
}

void ClothWorld::set_change_callback(ChangeCallback change_callback)
{
	change_callback_ = std::move(change_callback);
}

ClothObject* ClothWorld::find_cloth(ClothId cloth_id)
{
	return find_object<ClothObject>(cloth_id);
}

const ClothObject* ClothWorld::find_cloth(ClothId cloth_id) const
{
	return find_object<ClothObject>(cloth_id);
}

int ClothWorld::cloth_count() const
{
	return static_cast<int>(get_objects<ClothObject>().size());
}

std::string ClothWorld::make_default_cloth_name(ClothId cloth_id, ClothSolverKind solver_kind) const
{
	const char* solver_name = solver_kind == ClothSolverKind::XPBD ? "XPBD" : "PBD";
	return std::string(solver_name) + " Cloth " + std::to_string(cloth_id);
}

void ClothWorld::attach_solver(ClothObject& cloth_object, ClothSolverKind solver_kind)
{
	switch (solver_kind)
	{
	case ClothSolverKind::XPBD:
		cloth_object.add_component<XPBDClothSimulationComponent>(cloth_object.cloth());
		return;
	case ClothSolverKind::PBD:
	default:
		cloth_object.add_component<PBDClothSimulationComponent>(cloth_object.cloth());
		return;
	}
}

void ClothWorld::notify_snapshot_invalidated() const
{
	if (change_callback_)
	{
		change_callback_(ChangeEvent{ ChangeEvent::Kind::SnapshotInvalidated });
	}
}

void ClothWorld::notify_selection_changed() const
{
	if (change_callback_)
	{
		ChangeEvent event;
		event.kind = ChangeEvent::Kind::SelectionChanged;
		event.selected_cloth_id = selected_cloth_id_;
		change_callback_(event);
	}
}

void ClothWorld::notify_cloth_value_changed(
	ClothId cloth_id,
	ObjectId source_object_id,
	std::string value_id,
	PropertyValue value) const
{
	if (change_callback_)
	{
		ChangeEvent event;
		event.kind = ChangeEvent::Kind::ClothValueChanged;
		event.cloth_id = cloth_id;
		event.selected_cloth_id = selected_cloth_id_;
		event.source_object_id = source_object_id;
		event.value_id = std::move(value_id);
		event.value = std::move(value);
		change_callback_(event);
	}
}

void ClothWorld::on_component_property_changed(
	const WorldObject& owner,
	const Component& component,
	const PropertyBase& property)
{
	if (const ClothObject* cloth_object = dynamic_cast<const ClothObject*>(&owner))
	{
		if (dynamic_cast<const ClothSimulationComponentBase*>(&component))
		{
			notify_cloth_value_changed(cloth_object->cloth_id(), component.id(), std::string(property.id()), property.value());
			return;
		}
	}

	World::on_component_property_changed(owner, component, property);
}
