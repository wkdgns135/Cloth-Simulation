#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "cloth/core/Cloth.h"
#include "cloth/core/Particle.h"
#include "engine/core/WorldObject.h"

class ClothSimulationComponentBase;

using ClothId = ObjectId;

enum class ClothSourceKind
{
	Grid,
	Mesh,
};

enum class ClothSolverKind
{
	PBD,
	XPBD,
};

class ClothObject final : public WorldObject
{
public:
	ClothObject(std::string display_name, int width, int height, float spacing);
	ClothObject(std::string display_name, const std::filesystem::path& mesh_path);

	ClothId cloth_id() const { return id(); }
	const std::string& source_label() const { return source_label_; }
	ClothSourceKind source_kind() const { return source_kind_; }
	void set_anchors_enabled(bool enabled) { anchors_enabled_property_(enabled); }
	bool anchors_enabled() const { return anchors_enabled_property_(); }
	ClothSolverKind solver_kind() const;

	Cloth& cloth() { return cloth_; }
	const Cloth& cloth() const { return cloth_; }
	ClothSimulationComponentBase* simulation_component();
	const ClothSimulationComponentBase* simulation_component() const;

	void reset_to_initial_state();
	void toggle_anchor_state();
	bool hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const override;

protected:
	bool on_click(const ClickInputEvent& event) override;
	
private:
	void apply_anchor_state(bool enabled);
	void cache_initial_state();
	void refresh_initial_state_if_needed();

	std::string source_label_;
	ClothSourceKind source_kind_ = ClothSourceKind::Grid;
	Cloth cloth_;
	std::vector<Particle> initial_particles_;
	std::uint64_t cached_topology_revision_ = 0;
	Property<bool> anchors_enabled_property_{
		*this,
		make_property_config<bool>("anchors_enabled", "Anchors Enabled", "Cloth"),
		true,
		[this](const bool& enabled) { apply_anchor_state(enabled); } };
};
