#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/core/Component.h"
#include "engine/core/Transform.h"
#include "engine/lifecycle/Lifecycle.h"

struct RenderScene;

class Object : public Lifecycle
{
public:
	Object() = default;
	~Object() override = default;

	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;

	void awake() override;
	void start() override;
	void update(float delta_time) override;
	void stop() override;
	void destroy() override;

	void collect_render_data(RenderScene& scene) const;

	Transform& transform() { return transform_; }
	const Transform& transform() const { return transform_; }

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
		for (const std::unique_ptr<Component>& component : components_)
		{
			if (T* typed_component = dynamic_cast<T*>(component.get()))
			{
				result.push_back(typed_component);
			}
		}
		return result;
	}

	template <typename T>
	std::vector<const T*> get_components() const
	{
		static_assert(std::is_base_of_v<Component, T>);

		std::vector<const T*> result;
		for (const std::unique_ptr<Component>& component : components_)
		{
			if (const T* typed_component = dynamic_cast<const T*>(component.get()))
			{
				result.push_back(typed_component);
			}
		}
		return result;
	}

private:
	Transform transform_;
	std::vector<std::unique_ptr<Component>> components_;
	bool awakened_ = false;
	bool started_ = false;
	bool destroyed_ = false;
};
