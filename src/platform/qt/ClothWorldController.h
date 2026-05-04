#pragma once

#include <QObject>

#include <filesystem>

#include "cloth/ClothObject.h"

class Engine;

class ClothWorldController : public QObject
{
	Q_OBJECT

public:
	explicit ClothWorldController(Engine& engine, QObject* parent = nullptr);
	~ClothWorldController() override = default;

	void create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind);
	void create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind);
	void spawn_sphere_from_view();
	void reset_selected_cloth();
	void toggle_selected_cloth_anchors();
	void delete_selected_cloth();

private:
	Engine& engine_;
};
