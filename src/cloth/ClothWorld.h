#pragma once

#include <filesystem>
#include <string>

#include <glm/glm.hpp>

#include "cloth/ClothObject.h"
#include "engine/core/Property.h"
#include "engine/core/World.h"

class PlaneObject;
class SphereObject;

class ClothWorld final : public World
{
public:
	ClothWorld();

	ClothObject& create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind);
	ClothObject& create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind);
	PlaneObject& create_plane_object(const glm::vec3& position, const glm::vec3& scale = glm::vec3(8.0f, 1.0f, 8.0f));
	SphereObject& create_sphere_object(float radius, const glm::vec3& position);
	void spawn_sphere_projectile();
	bool destroy_cloth(ClothId cloth_id);
	bool reset_cloth(ClothId cloth_id);
	bool toggle_cloth_anchors(ClothId cloth_id);
	bool reset_selected_cloth();
	bool toggle_selected_cloth_anchors();
	bool delete_selected_cloth();
	ClothId selected_cloth_id() const;

private:
	ClothObject* find_cloth(ClothId cloth_id);
	const ClothObject* find_cloth(ClothId cloth_id) const;
	int cloth_count() const;
	std::string make_default_cloth_name(ClothId cloth_id, ClothSolverKind solver_kind) const;
	void attach_solver(ClothObject& cloth_object, ClothSolverKind solver_kind);
};
