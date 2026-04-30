#include "engine/core/WorldObject.h"

#include "engine/components/RenderComponent.h"
#include "engine/core/World.h"

void WorldObject::add_component(std::unique_ptr<Component> component)
{
	if (!component)
	{
		return;
	}

	component->set_owner(this);
	Component& result = *component;
	components_.insert(std::move(component));

	if (awakened_)
	{
		result.awake();
	}
	if (started_)
	{
		result.start();
	}
}

void WorldObject::awake()
{
	if (awakened_)
	{
		return;
	}

	awakened_ = true;
	destroyed_ = false;

	for (const std::unique_ptr<Component>& component : components_.ordered())
	{
		component->awake();
	}
}

void WorldObject::start()
{
	if (started_)
	{
		return;
	}

	awake();
	started_ = true;

	for (const std::unique_ptr<Component>& component : components_.ordered())
	{
		component->start();
	}
}

void WorldObject::update(float delta_time)
{
	for (const std::unique_ptr<Component>& component : components_.ordered())
	{
		component->update(delta_time);
	}
}

void WorldObject::stop()
{
	if (!started_)
	{
		return;
	}

	started_ = false;

	for (const std::unique_ptr<Component>& component : components_.ordered())
	{
		component->stop();
	}
}

void WorldObject::destroy()
{
	if (destroyed_)
	{
		return;
	}

	stop();

	for (const std::unique_ptr<Component>& component : components_.ordered())
	{
		component->destroy();
	}

	destroyed_ = true;
	awakened_ = false;
	destroy_requested_ = false;
}

void WorldObject::collect_render_data(RenderScene& scene) const
{
	for (const std::unique_ptr<Component>& component : components_.ordered())
	{
		if (const RenderComponent* render_component = dynamic_cast<const RenderComponent*>(component.get()))
		{
			render_component->collect_render_data(scene);
		}
	}
}

bool WorldObject::on_click(const ClickInputEvent& event)
{
	static_cast<void>(event);
	return false;
}

void WorldObject::on_hover_enter(const PointerPosition& position)
{
	static_cast<void>(position);
}

void WorldObject::on_hover_leave(const PointerPosition& position)
{
	static_cast<void>(position);
}

bool WorldObject::hit_test(const glm::vec3& ray_origin, const glm::vec3& ray_direction, float& hit_distance) const
{
	static_cast<void>(ray_origin);
	static_cast<void>(ray_direction);
	static_cast<void>(hit_distance);
	return false;
}

void WorldObject::request_destroy()
{
	if (destroy_requested_ || !world_)
	{
		return;
	}

	destroy_requested_ = true;
	world_->request_destroy_object(this);
}
