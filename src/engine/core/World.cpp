#include "engine/core/World.h"

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
