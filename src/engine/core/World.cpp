#include "engine/core/World.h"

#include <glm/geometric.hpp>

#include "engine/objects/CameraObject.h"
#include "engine/objects/DirectionalLightObject.h"

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

World::World()
{
	CameraObject& default_camera = create_object<CameraObject>();
	default_camera.transform().position = glm::vec3(0.0f, 0.0f, 2.0f);
	default_camera.transform().look_at(glm::vec3(0.0f));
	set_main_camera(default_camera);

	DirectionalLightObject& default_light = create_object<DirectionalLightObject>();
	default_light.transform().set_forward(glm::normalize(glm::vec3(-0.35f, 0.65f, 0.70f)));
	set_main_directional_light(default_light);
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

CameraObject& World::main_camera()
{
	return *main_camera_object_;
}

const CameraObject& World::main_camera() const
{
	return *main_camera_object_;
}

DirectionalLightObject& World::main_directional_light()
{
	return *main_directional_light_object_;
}

const DirectionalLightObject& World::main_directional_light() const
{
	return *main_directional_light_object_;
}

void World::set_main_camera(CameraObject& camera_object)
{
	main_camera_object_ = &camera_object;
}

void World::set_main_directional_light(DirectionalLightObject& directional_light_object)
{
	main_directional_light_object_ = &directional_light_object;
}

RenderScene World::build_render_scene() const
{
	RenderScene scene;

	if (main_camera_object_)
	{
		scene.camera = main_camera_object_->build_camera();
	}

	if (main_directional_light_object_)
	{
		scene.directional_light = main_directional_light_object_->build_light();
	}

	for (const std::unique_ptr<Object>& object : objects_)
	{
		object->collect_render_data(scene);
	}

	return scene;
}
