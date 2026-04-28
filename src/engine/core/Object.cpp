#include "engine/core/Object.h"

#include "engine/components/InputComponent.h"
#include "engine/components/RenderComponent.h"

namespace
{
template <typename Callback>
bool dispatch_input_event(std::vector<std::unique_ptr<Component>>& components, Callback&& callback)
{
	for (auto it = components.rbegin(); it != components.rend(); ++it)
	{
		if (InputComponent* input_component = dynamic_cast<InputComponent*>(it->get()))
		{
			if (callback(*input_component))
			{
				return true;
			}
		}
	}

	return false;
}
}

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

bool Object::handle_key_pressed(const KeyInputEvent& event)
{
	return dispatch_input_event(components_, [&](InputComponent& input_component) {
		return input_component.on_key_pressed(event);
	});
}

bool Object::handle_key_released(const KeyInputEvent& event)
{
	return dispatch_input_event(components_, [&](InputComponent& input_component) {
		return input_component.on_key_released(event);
	});
}

bool Object::handle_pointer_pressed(const PointerInputEvent& event)
{
	return dispatch_input_event(components_, [&](InputComponent& input_component) {
		return input_component.on_pointer_pressed(event);
	});
}

bool Object::handle_pointer_released(const PointerInputEvent& event)
{
	return dispatch_input_event(components_, [&](InputComponent& input_component) {
		return input_component.on_pointer_released(event);
	});
}

bool Object::handle_pointer_moved(const PointerInputEvent& event)
{
	return dispatch_input_event(components_, [&](InputComponent& input_component) {
		return input_component.on_pointer_moved(event);
	});
}

bool Object::handle_click(const ClickInputEvent& event)
{
	return dispatch_input_event(components_, [&](InputComponent& input_component) {
		return input_component.on_click(event);
	});
}

bool Object::handle_wheel_scrolled(const WheelInputEvent& event)
{
	return dispatch_input_event(components_, [&](InputComponent& input_component) {
		return input_component.on_wheel_scrolled(event);
	});
}
