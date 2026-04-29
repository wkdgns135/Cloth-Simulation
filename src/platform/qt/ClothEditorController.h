#pragma once

#include <QObject>
#include <QString>

#include <cstdint>
#include <filesystem>
#include <vector>

#include "cloth/ClothObject.h"

class Engine;

class ClothEditorController : public QObject
{
	Q_OBJECT

public:
	struct ClothViewState
	{
		std::uint64_t id = 0;
		QString name;
		QString source_label;
		QString source_kind_label;
		QString solver_label;
		int particle_count = 0;
		int triangle_count = 0;
		bool anchors_enabled = false;
		float position_x = 0.0f;
		float position_y = 0.0f;
		float position_z = 0.0f;
		float damping = 0.99f;
		int constraint_iterations = 20;
	};

	struct WorldViewState
	{
		std::vector<ClothViewState> cloths;
		std::uint64_t selected_cloth_id = 0;
	};

	explicit ClothEditorController(Engine& engine, QObject* parent = nullptr);
	~ClothEditorController() override;

	const WorldViewState& snapshot() const { return snapshot_; }
	const ClothViewState* selected_cloth() const;

	void create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind);
	void create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind);
	void spawn_sphere_from_view();
	void set_selected_cloth(std::uint64_t cloth_id);
	void update_selected_cloth_position(float x, float y, float z);
	void update_selected_cloth_simulation_settings(float damping, int constraint_iterations);
	void reset_selected_cloth();
	void toggle_selected_cloth_anchors();
	void delete_selected_cloth();

signals:
	void snapshot_updated();

private:
	void bind_world_notifications();
	void request_snapshot();
	void apply_snapshot(WorldViewState snapshot);

	Engine& engine_;
	WorldViewState snapshot_;
	bool snapshot_request_pending_ = false;
};
