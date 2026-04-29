#include "cloth/ClothWorld.h"

#include <glm/geometric.hpp>

#include "cloth/components/ClothCameraInputComponent.h"
#include "cloth/components/ClothSimulationComponentBase.h"
#include "cloth/components/PBDClothSimulationComponent.h"
#include "cloth/components/XPBDClothSimulationComponent.h"
#include "engine/objects/CameraObject.h"
#include "engine/objects/DirectionalLightObject.h"

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
}

ClothObject& ClothWorld::create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind)
{
	const int existing_cloth_count = cloth_count();
	const ClothId cloth_id = allocate_cloth_id();
	ClothObject& cloth_object = create_object<ClothObject>(
		cloth_id,
		make_default_cloth_name(cloth_id, solver_kind),
		width,
		height,
		spacing);
	attach_solver(cloth_object, solver_kind);
	cloth_object.transform().position = glm::vec3(0.35f * static_cast<float>(existing_cloth_count), 0.0f, 0.0f);
	selected_cloth_id_ = cloth_id;
	notify_changed();
	return cloth_object;
}

ClothObject& ClothWorld::create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind)
{
	const int existing_cloth_count = cloth_count();
	const ClothId cloth_id = allocate_cloth_id();
	ClothObject& cloth_object = create_object<ClothObject>(
		cloth_id,
		make_default_cloth_name(cloth_id, solver_kind),
		mesh_path);
	attach_solver(cloth_object, solver_kind);
	cloth_object.transform().position = glm::vec3(0.35f * static_cast<float>(existing_cloth_count), 0.0f, 0.0f);
	selected_cloth_id_ = cloth_id;
	notify_changed();
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

	notify_changed();
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
		notify_changed();
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
	notify_changed();
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
	notify_changed();
	return true;
}

bool ClothWorld::set_cloth_simulation_settings(ClothId cloth_id, float damping, int constraint_iterations)
{
	ClothObject* cloth_object = find_cloth(cloth_id);
	if (!cloth_object)
	{
		return false;
	}

	ClothSimulationComponentBase* simulation_component = cloth_object->simulation_component();
	if (!simulation_component)
	{
		return false;
	}

	simulation_component->set_damping(damping);
	simulation_component->set_constraint_iterations(constraint_iterations);
	notify_changed();
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
	notify_changed();
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
	notify_changed();
	return true;
}

void ClothWorld::set_change_callback(ChangeCallback change_callback)
{
	change_callback_ = std::move(change_callback);
}

ClothId ClothWorld::allocate_cloth_id()
{
	return next_cloth_id_++;
}

ClothObject* ClothWorld::find_cloth(ClothId cloth_id)
{
	for (ClothObject* cloth_object : get_objects<ClothObject>())
	{
		if (cloth_object->cloth_id() == cloth_id)
		{
			return cloth_object;
		}
	}

	return nullptr;
}

const ClothObject* ClothWorld::find_cloth(ClothId cloth_id) const
{
	for (const ClothObject* cloth_object : get_objects<ClothObject>())
	{
		if (cloth_object->cloth_id() == cloth_id)
		{
			return cloth_object;
		}
	}

	return nullptr;
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

void ClothWorld::notify_changed() const
{
	if (change_callback_)
	{
		change_callback_();
	}
}
