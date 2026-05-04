#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/core/Component.h"
#include "engine/core/IndexedStore.h"

struct RenderScene;
struct ClickInputEvent;
struct PointerPosition;
class World;
class TransformComponent;

class WorldObject : public Object
{
public:
	WorldObject();
	explicit WorldObject(std::string display_name);
	~WorldObject() override = default;

	WorldObject(const WorldObject&) = delete;
	WorldObject& operator=(const WorldObject&) = delete;

	void awake() override;
	void start() override;
	void update(float delta_time) override;
	void stop() override;
	void destroy() override;

	void collect_render_data(RenderScene& scene) const;
	virtual int update_order() const { return 0; }
	virtual bool on_click(const ClickInputEvent& event);
	virtual void on_hover_enter(const PointerPosition& position);
	virtual void on_hover_leave(const PointerPosition& position);
	virtual bool hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const;
	void request_destroy();
	bool destroy_requested() const { return destroy_requested_; }

	TransformComponent& transform();
	const TransformComponent& transform() const;
	glm::vec3 get_object_world_position() const;
	void set_object_world_position(const glm::vec3& position);
	glm::vec3 get_object_rotation() const;
	void set_object_rotation(const glm::vec3& rotation);
	glm::vec3 get_object_scale() const;
	void set_object_scale(const glm::vec3& scale);
	glm::mat4 get_object_transform_matrix() const;
	glm::vec3 get_object_forward_direction() const;
	glm::vec3 get_object_up_direction() const;
	glm::vec3 get_object_right_direction() const;
	void set_object_forward_direction(const glm::vec3& direction);
	void look_at_world_position(const glm::vec3& target);
	void set_world(World* world) { world_ = world; }
	World* world() { return world_; }
	const World* world() const { return world_; }
	IndexedStore<Component>::Storage& components() { return components_.ordered(); }
	const IndexedStore<Component>::Storage& components() const { return components_.ordered(); }

	void add_component(std::unique_ptr<Component> component);

	template <typename T, typename... Args>
	T& add_component(Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>);

		std::unique_ptr<Component> component = std::make_unique<T>(std::forward<Args>(args)...);
		T& result = static_cast<T&>(*component);
		add_component(std::move(component));
		return result;
	}

	template <typename T>
	std::vector<T*> get_components()
	{
		static_assert(std::is_base_of_v<Component, T>);

		std::vector<T*> result;
		for (const std::unique_ptr<Component>& component : components_.ordered())
		{
			if (T* typed_component = dynamic_cast<T*>(component.get()))
			{
				result.push_back(typed_component);
			}
		}
		return result;
	}

	template <typename T>
	T* find_component()
	{
		static_assert(std::is_base_of_v<Component, T>);

		for (const std::unique_ptr<Component>& component : components_.ordered())
		{
			if (T* typed_component = dynamic_cast<T*>(component.get()))
			{
				return typed_component;
			}
		}

		return nullptr;
	}

	template <typename T>
	std::vector<const T*> get_components() const
	{
		static_assert(std::is_base_of_v<Component, T>);

		std::vector<const T*> result;
		for (const std::unique_ptr<Component>& component : components_.ordered())
		{
			if (const T* typed_component = dynamic_cast<const T*>(component.get()))
			{
				result.push_back(typed_component);
			}
		}
		return result;
	}

	template <typename T>
	const T* find_component() const
	{
		static_assert(std::is_base_of_v<Component, T>);

		for (const std::unique_ptr<Component>& component : components_.ordered())
		{
			if (const T* typed_component = dynamic_cast<const T*>(component.get()))
			{
				return typed_component;
			}
		}

		return nullptr;
	}

	template <typename T>
	T* find_component(ObjectId component_id)
	{
		static_assert(std::is_base_of_v<Component, T>);
		return dynamic_cast<T*>(components_.find(component_id));
	}

	template <typename T>
	const T* find_component(ObjectId component_id) const
	{
		static_assert(std::is_base_of_v<Component, T>);
		return dynamic_cast<const T*>(components_.find(component_id));
	}

protected:
	void on_property_changed(const PropertyBase& property) override;
	void on_component_property_changed(const Component& component, const PropertyBase& property);

private:
	friend class Component;

	World* world_ = nullptr;
	IndexedStore<Component> components_;
	TransformComponent* transform_component_ = nullptr;
	bool awakened_ = false;
	bool started_ = false;
	bool destroyed_ = false;
	bool destroy_requested_ = false;
};
