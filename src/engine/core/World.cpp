#include "engine/core/World.h"

namespace
{
template <typename Callback>
bool dispatch_input_to_objects(std::vector<std::unique_ptr<Object>>& objects, Callback&& callback)
{
	for (auto it = objects.rbegin(); it != objects.rend(); ++it)
	{
		if (callback(*(*it)))
		{
			return true;
		}
	}

	return false;
}
}

void World::awake()
{
	if (awakened_)
	{
		return;
	}

	awakened_ = true;
	destroyed_ = false;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->awake();
	}
}

void World::start()
{
	if (started_)
	{
		return;
	}

	awake();
	started_ = true;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->start();
	}
}

void World::update(float delta_time)
{
	for (std::size_t i = 0; i < objects_.size(); ++i)
	{
		objects_[i]->update(delta_time);
	}
}

void World::stop()
{
	if (!started_)
	{
		return;
	}

	started_ = false;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->stop();
	}
}

void World::destroy()
{
	if (destroyed_)
	{
		return;
	}

	stop();

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->destroy();
	}

	destroyed_ = true;
	awakened_ = false;
}

bool World::handle_key_pressed(const KeyInputEvent& event)
{
	return dispatch_input_to_objects(objects_, [&](Object& object) {
		return object.handle_key_pressed(event);
	});
}

bool World::handle_key_released(const KeyInputEvent& event)
{
	return dispatch_input_to_objects(objects_, [&](Object& object) {
		return object.handle_key_released(event);
	});
}

bool World::handle_pointer_pressed(const PointerInputEvent& event)
{
	return dispatch_input_to_objects(objects_, [&](Object& object) {
		return object.handle_pointer_pressed(event);
	});
}

bool World::handle_pointer_released(const PointerInputEvent& event)
{
	return dispatch_input_to_objects(objects_, [&](Object& object) {
		return object.handle_pointer_released(event);
	});
}

bool World::handle_pointer_moved(const PointerInputEvent& event)
{
	return dispatch_input_to_objects(objects_, [&](Object& object) {
		return object.handle_pointer_moved(event);
	});
}

bool World::handle_click(const ClickInputEvent& event)
{
	return dispatch_input_to_objects(objects_, [&](Object& object) {
		return object.handle_click(event);
	});
}

bool World::handle_wheel_scrolled(const WheelInputEvent& event)
{
	return dispatch_input_to_objects(objects_, [&](Object& object) {
		return object.handle_wheel_scrolled(event);
	});
}

RenderScene World::build_render_scene() const
{
	RenderScene scene;
	scene.camera = camera_;
	scene.directional_light = directional_light_;

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->collect_render_data(scene);
	}

	return scene;
}
