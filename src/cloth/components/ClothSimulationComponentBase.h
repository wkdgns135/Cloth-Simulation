#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "engine/components/SimulationComponent.h"

class Cloth;

class ClothSimulationComponentBase : public SimulationComponent
{
public:
	struct DistanceConstraint
	{
		enum class Kind
		{
			Stretch,
			Shear,
			Bend,
		};

		int particle_a = 0;
		int particle_b = 0;
		float rest_length = 0.0f;
		Kind kind = Kind::Stretch;
	};

	struct DistanceConstraintEvaluation
	{
		glm::vec3 gradient = glm::vec3(0.0f);
		float constraint_value = 0.0f;
		float inverse_mass_a = 0.0f;
		float inverse_mass_b = 0.0f;
		float denominator = 0.0f;
	};

	using SpatialHashCell = std::array<int, 3>;

	struct SpatialHashCellHasher
	{
		std::size_t operator()(const SpatialHashCell& cell) const;
	};

	explicit ClothSimulationComponentBase(Cloth& cloth);

	void start() override;
	void set_external_acceleration(const glm::vec3& acceleration) { external_acceleration_ = acceleration; }
	void add_external_acceleration(const glm::vec3& acceleration) { external_acceleration_ += acceleration; }
	const glm::vec3& external_acceleration() const { return external_acceleration_; }

	PROPERTY(glm::vec3, gravity, "Simulation", "Gravity", glm::vec3(0.0f, -9.8f, 0.0f))
	PROPERTY_RANGE(float, damping, "Simulation", "Damping", 0.99f, 0.0f, 1.0f, 0.01f)
	PROPERTY_RANGE(int, substeps, "Simulation", "Substeps", 10, 1, 64, 1)
	PROPERTY_RANGE(int, constraint_iterations, "Simulation", "Constraint Iterations", 1, 1, 64, 1)
	PROPERTY(bool, external_collision_enabled, "Simulation", "External Collision Enabled", true)
	PROPERTY(bool, self_collision_enabled, "Simulation", "Self Collision Enabled", true)
	PROPERTY_RANGE(float, collision_margin, "Simulation", "Collision Margin", 0.05f, 0.0f, 1.0f, 0.001f)
	
protected:
	Cloth& cloth() { return cloth_; }
	const Cloth& cloth() const { return cloth_; }

	void integrate(float delta_time);
	void clear_external_acceleration() { external_acceleration_ = glm::vec3(0.0f); }
	int solver_pass_count() const;
	bool rebuild_constraints();
	bool rebuild_constraints_if_needed();

	const std::vector<DistanceConstraint>& distance_constraints() const { return distance_constraints_; }

	bool evaluate_distance_constraint(
		const DistanceConstraint& constraint,
		DistanceConstraintEvaluation& evaluation) const;

	float per_iteration_stiffness(float stiffness) const;
	void build_spatial_hash();
	void prepare_self_collision_candidates(float frame_delta_time);
	
	void solve_collision_objects();
	void solve_self_collision();

private:
	float max_self_collision_displacement_per_pass() const;

	glm::vec3 world_to_local_point(const glm::vec3& world_point) const;
	glm::vec3 local_to_world_point(const glm::vec3& local_point) const;

	Cloth& cloth_;
	std::vector<DistanceConstraint> distance_constraints_;
	std::vector<glm::vec3> rest_positions_;
	std::vector<std::vector<unsigned int>> self_collision_candidate_ids_;
	glm::vec3 external_acceleration_ = glm::vec3(0.0f);
	std::uint64_t cached_topology_revision_ = 0;
	std::unordered_map<SpatialHashCell, std::vector<unsigned int>, SpatialHashCellHasher> spatial_hash_;
};
