#pragma once

#include <filesystem>
#include <functional>
#include <optional>
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
	struct ChangeEvent
	{
		enum class Kind
		{
			SnapshotInvalidated,
			SelectionChanged,
			ClothValueChanged,
		};

		Kind kind = Kind::SnapshotInvalidated;
		ClothId cloth_id = 0;
		ClothId selected_cloth_id = 0;
		ObjectId source_object_id = 0;
		std::string value_id;
		std::optional<PropertyValue> value;
	};

	using ChangeCallback = std::function<void(const ChangeEvent&)>;

	ClothWorld();

	ClothObject& create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind);
	ClothObject& create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind);
	PlaneObject& create_plane_object(const glm::vec3& position, const glm::vec3& scale = glm::vec3(8.0f, 1.0f, 8.0f));
	SphereObject& create_sphere_object(float radius, const glm::vec3& position);
	void spawn_sphere_projectile();
	bool destroy_cloth(ClothId cloth_id);
	bool select_cloth(ClothId cloth_id);
	bool set_cloth_property(ClothId cloth_id, ObjectId source_object_id, std::string property_id, const PropertyValue& value);
	bool reset_cloth(ClothId cloth_id);
	bool toggle_cloth_anchors(ClothId cloth_id);
	void set_change_callback(ChangeCallback change_callback);
	ClothId selected_cloth_id() const { return selected_cloth_id_; }

private:
	ClothObject* find_cloth(ClothId cloth_id);
	const ClothObject* find_cloth(ClothId cloth_id) const;
	int cloth_count() const;
	std::string make_default_cloth_name(ClothId cloth_id, ClothSolverKind solver_kind) const;
	void attach_solver(ClothObject& cloth_object, ClothSolverKind solver_kind);
	void notify_snapshot_invalidated() const;
	void notify_selection_changed() const;
	void notify_cloth_value_changed(ClothId cloth_id, ObjectId source_object_id, std::string value_id, PropertyValue value) const;
	void on_world_object_property_changed(const WorldObject& object, const PropertyBase& property) override;
	void on_component_property_changed(const WorldObject& owner, const Component& component, const PropertyBase& property) override;

	ClothId selected_cloth_id_ = 0;
	ChangeCallback change_callback_;
};
