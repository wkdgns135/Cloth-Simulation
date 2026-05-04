#pragma once

#include <QObject>
#include <QString>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include "cloth/ClothWorld.h"
#include "engine/core/Property.h"

class Engine;

class ClothEditorController : public QObject
{
	Q_OBJECT

public:
	struct PropertyViewState
	{
		std::uint64_t source_object_id = 0;
		QString source_label;
		QString id;
		QString label;
		QString group;
		PropertyType type = PropertyType::Float;
		bool editable = true;
		PropertyValue value = false;
		std::optional<PropertyValue> min_value;
		std::optional<PropertyValue> max_value;
		std::optional<PropertyValue> step;
	};

	struct ObjectViewState
	{
		std::uint64_t id = 0;
		QString name;
		QString type_label;
		QString detail_label;
		QString metrics_label;
		bool is_cloth = false;
		std::vector<PropertyViewState> properties;
	};

	struct WorldViewState
	{
		std::vector<ObjectViewState> objects;
		std::uint64_t selected_object_id = 0;
	};

	explicit ClothEditorController(Engine& engine, QObject* parent = nullptr);
	~ClothEditorController() override;

	const WorldViewState& snapshot() const { return snapshot_; }
	const ObjectViewState* selected_object() const;
	const ObjectViewState* selected_cloth() const;

	void create_grid_cloth(int width, int height, float spacing, ClothSolverKind solver_kind);
	void create_mesh_cloth(const std::filesystem::path& mesh_path, ClothSolverKind solver_kind);
	void spawn_sphere_from_view();
	void set_selected_object(std::uint64_t object_id);
	void update_selected_object_property(std::uint64_t source_object_id, const QString& property_id, const PropertyValue& value);
	void reset_selected_cloth();
	void toggle_selected_cloth_anchors();
	void delete_selected_cloth();

signals:
	void snapshot_updated();
	void selected_object_value_updated(QString value_id);

private:
	void bind_world_notifications();
	void handle_world_change(const ClothWorld::ChangeEvent& change);
	void request_snapshot();
	void apply_snapshot(WorldViewState snapshot);
	ObjectViewState* find_object_view(std::uint64_t object_id);
	bool apply_object_value_change(const ClothWorld::ChangeEvent& change);

	Engine& engine_;
	WorldViewState snapshot_;
	bool snapshot_request_pending_ = false;
	bool snapshot_refresh_requested_ = false;
};
