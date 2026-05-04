#include "engine/core/WorldObject.h"

#include "engine/components/RenderComponent.h"
#include "engine/components/TransformComponent.h"
#include "engine/core/World.h"

WorldObject::WorldObject()
{
	transform_component_ = &add_component<TransformComponent>();
}

WorldObject::WorldObject(std::string display_name)
	: Object(std::move(display_name))
{
	transform_component_ = &add_component<TransformComponent>();
}

TransformComponent& WorldObject::transform()
{
	return *transform_component_;
}

const TransformComponent& WorldObject::transform() const
{
	return *transform_component_;
}

glm::vec3 WorldObject::get_object_world_position() const
{
	return transform().position();
}

void WorldObject::set_object_world_position(const glm::vec3& position)
{
	transform().set_position(position);
}

glm::vec3 WorldObject::get_object_rotation() const
{
	return transform().rotation();
}

void WorldObject::set_object_rotation(const glm::vec3& rotation)
{
	transform().set_rotation(rotation);
}

glm::vec3 WorldObject::get_object_scale() const
{
	return transform().scale();
}

void WorldObject::set_object_scale(const glm::vec3& scale)
{
	transform().set_scale(scale);
}

glm::mat4 WorldObject::get_object_transform_matrix() const
{
	return transform().matrix();
}

glm::vec3 WorldObject::get_object_forward_direction() const
{
	return transform().forward();
}

glm::vec3 WorldObject::get_object_up_direction() const
{
	return transform().up();
}

glm::vec3 WorldObject::get_object_right_direction() const
{
	return transform().right();
}

void WorldObject::set_object_forward_direction(const glm::vec3& direction)
{
	transform().set_forward(direction);
}

void WorldObject::look_at_world_position(const glm::vec3& target)
{
	transform().look_at(target);
}

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

void WorldObject::on_property_changed(const PropertyBase& property)
{
	if (world_)
	{
		world_->notify_world_object_property_changed(*this, property);
		return;
	}

	Object::on_property_changed(property);
}

void WorldObject::on_component_property_changed(const Component& component, const PropertyBase& property)
{
	if (world_)
	{
		world_->notify_component_property_changed(*this, component, property);
		return;
	}

	Object::on_property_changed(property);
}
