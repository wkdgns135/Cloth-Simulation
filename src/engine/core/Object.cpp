#include "engine/core/Object.h"

#include "engine/render/RenderComponent.h"

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
