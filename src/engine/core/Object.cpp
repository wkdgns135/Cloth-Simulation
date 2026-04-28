#include "engine/core/Object.h"

#include "engine/components/RenderComponent.h"

void Object::add_component(std::unique_ptr<Component> component)
{
	if (!component)
	{
		return;
	}

	component->set_owner(this);
	Component& result = *component;
	components_.push_back(std::move(component));

	if (awakened_)
	{
		result.awake();
	}
	if (started_)
	{
		result.start();
	}
}

void Object::awake()
{
	if (awakened_)
	{
		return;
	}

	awakened_ = true;
	destroyed_ = false;

	for (const std::unique_ptr<Component>& component : components_)
	{
		component->awake();
	}
}

void Object::start()
{
	if (started_)
	{
		return;
	}

	awake();
	started_ = true;

	for (const std::unique_ptr<Component>& component : components_)
	{
		component->start();
	}
}

void Object::update(float delta_time)
{
	for (const std::unique_ptr<Component>& component : components_)
	{
		component->update(delta_time);
	}
}

void Object::stop()
{
	if (!started_)
	{
		return;
	}

	started_ = false;

	for (const std::unique_ptr<Component>& component : components_)
	{
		component->stop();
	}
}

void Object::destroy()
{
	if (destroyed_)
	{
		return;
	}

	stop();

	for (const std::unique_ptr<Component>& component : components_)
	{
		component->destroy();
	}

	destroyed_ = true;
	awakened_ = false;
}

void Object::collect_render_data(RenderScene& scene) const
{
	for (const std::unique_ptr<Component>& component : components_)
	{
		if (const RenderComponent* render_component = dynamic_cast<const RenderComponent*>(component.get()))
		{
			render_component->collect_render_data(scene);
		}
	}
}

bool Object::on_click(const ClickInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

void Object::on_hover_enter(const PointerPosition& position)
{
	static_cast<void>(position);
}

void Object::on_hover_leave(const PointerPosition& position)
{
	static_cast<void>(position);
}

bool Object::hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const
{
	static_cast<void>(ray_origin);
	static_cast<void>(ray_direction);
	static_cast<void>(hit_distance);
	return false;
}
