#include "cloth/ClothWorld.h"

#include <algorithm>
#include <limits>

#include <glm/geometric.hpp>

#include "cloth/components/ClothCameraInputComponent.h"
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

	const glm::mat4 object_transform = cloth_object.get_object_transform_matrix();
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
	main_camera_object_->set_object_world_position(glm::vec3(0.0f, 0.0f, 2.6f));
	main_camera_object_->look_at_world_position(glm::vec3(0.0f, 0.0f, 0.0f));
	main_camera_object_->set_fov_y_degrees(45.0f);
	main_camera_object_->add_component<ClothCameraInputComponent>();

	main_directional_light_object_->set_object_forward_direction(glm::normalize(glm::vec3(-0.75f, 0.65f, 0.45f)));
	main_directional_light_object_->set_ambient_strength(0.16f);
	main_directional_light_object_->set_diffuse_strength(1.15f);

	ClothObject& initial_cloth = create_mesh_cloth("asset/test_cloth_patch.obj", ClothSolverKind::PBD);
	initial_cloth.set_object_world_position(glm::vec3(0.0f, 0.0f, 0.0f));

	PlaneObject& floor_object = create_object<PlaneObject>();
	floor_object.set_object_world_position(glm::vec3(0.0f, compute_default_floor_height(initial_cloth), 0.0f));
	floor_object.set_object_scale(glm::vec3(8.0f, 1.0f, 8.0f));
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
	cloth_object.set_object_world_position(glm::vec3(0.35f * static_cast<float>(existing_cloth_count), 0.0f, 0.0f));
	select_object(cloth_object.id());
	return cloth_object;
}

PlaneObject& ClothWorld::create_plane_object(const glm::vec3& position, const glm::vec3& scale)
{
	PlaneObject& plane_object = create_object<PlaneObject>();
	plane_object.set_object_world_position(position);
	plane_object.set_object_scale(scale);
	return plane_object;
}

SphereObject& ClothWorld::create_sphere_object(float radius, const glm::vec3& position)
{
	SphereObject& sphere_object = create_object<SphereObject>(radius);
	sphere_object.set_object_world_position(position);
	return sphere_object;
}

void ClothWorld::spawn_sphere_projectile()
{
	if (!main_camera_object_)
	{
		return;
	}

	const glm::vec3 forward = main_camera_object_->get_object_forward_direction();
	const glm::vec3 spawn_position = main_camera_object_->get_object_world_position() + forward * kSpawnSphereForwardOffset;

	SphereObject& sphere_object = create_object<SphereObject>(kSpawnSphereRadius);
	sphere_object.set_object_world_position(spawn_position);
	sphere_object.configure_projectile(forward * kSpawnSphereSpeed, kSpawnSphereMaxTravelDistance);
}

ClothObject& ClothWorld::create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind)
{
	const int existing_cloth_count = cloth_count();
	ClothObject& cloth_object = create_object<ClothObject>(
		std::string(),
		mesh_path);
	cloth_object.set_display_name(make_default_cloth_name(cloth_object.id(), solver_kind));
	attach_solver(cloth_object, solver_kind);
	cloth_object.set_object_world_position(glm::vec3(0.35f * static_cast<float>(existing_cloth_count), 0.0f, 0.0f));
	select_object(cloth_object.id());
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

	return true;
}

bool ClothWorld::reset_cloth(ClothId cloth_id)
{
	ClothObject* cloth_object = find_cloth(cloth_id);
	if (!cloth_object)
	{
		return false;
	}

	cloth_object->reset_to_initial_state();
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
	return true;
}

bool ClothWorld::reset_selected_cloth()
{
	const ClothId cloth_id = selected_cloth_id();
	return cloth_id != 0 && reset_cloth(cloth_id);
}

bool ClothWorld::toggle_selected_cloth_anchors()
{
	const ClothId cloth_id = selected_cloth_id();
	return cloth_id != 0 && toggle_cloth_anchors(cloth_id);
}

bool ClothWorld::delete_selected_cloth()
{
	const ClothId cloth_id = selected_cloth_id();
	return cloth_id != 0 && destroy_cloth(cloth_id);
}

ClothId ClothWorld::selected_cloth_id() const
{
	const ClothObject* selected_cloth = find_object<ClothObject>(selected_object_id());
	return selected_cloth ? selected_cloth->id() : 0;
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
