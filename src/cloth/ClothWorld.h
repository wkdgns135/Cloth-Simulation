#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include <glm/glm.hpp>

#include "cloth/ClothObject.h"
#include "engine/core/World.h"

class ClothWorld final : public World
{
public:
	using ChangeCallback = std::function<void()>;

	ClothWorld();

	ClothObject& create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind);
	ClothObject& create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind);
	bool destroy_cloth(ClothId cloth_id);
	bool select_cloth(ClothId cloth_id);
	bool set_cloth_position(ClothId cloth_id, const glm::vec3& position);
	bool set_cloth_simulation_settings(ClothId cloth_id, float damping, int constraint_iterations);
	bool reset_cloth(ClothId cloth_id);
	bool toggle_cloth_anchors(ClothId cloth_id);
	void set_change_callback(ChangeCallback change_callback);
	ClothId selected_cloth_id() const { return selected_cloth_id_; }

private:
	ClothId allocate_cloth_id();
	ClothObject* find_cloth(ClothId cloth_id);
	const ClothObject* find_cloth(ClothId cloth_id) const;
	int cloth_count() const;
	std::string make_default_cloth_name(ClothId cloth_id, ClothSolverKind solver_kind) const;
	void attach_solver(ClothObject& cloth_object, ClothSolverKind solver_kind);
	void notify_changed() const;

	ClothId next_cloth_id_ = 1;
	ClothId selected_cloth_id_ = 0;
	ChangeCallback change_callback_;
};
