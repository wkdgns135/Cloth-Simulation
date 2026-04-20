#include "cloth/ClothWorld.h"

#include <utility>

void ClothWorld::initialize()
{
	std::lock_guard<std::mutex> lock(cloth_mutex_);

	if (initialized_)
	{
		return;
	}

	cloth_.build_grid(20, 20, 0.05f);
	cloth_.set_fixed(0, 0, true);
	cloth_.set_fixed(cloth_.get_width() - 1, 0, true);
	initialized_ = true;
}

void ClothWorld::step_physics(ClothSimulator& simulator, float delta_time)
{
	std::lock_guard<std::mutex> lock(cloth_mutex_);
	simulator.step(cloth_, delta_time);
}

void ClothWorld::publish_render_data()
{
	ClothRenderData next_render_data;

	{
		std::lock_guard<std::mutex> lock(cloth_mutex_);
		next_render_data.positions = cloth_.get_positions();
	}

	{
		std::lock_guard<std::mutex> lock(render_data_mutex_);
		render_data_ = std::move(next_render_data);
		render_data_dirty_ = true;
	}
}

bool ClothWorld::consume_render_data(ClothRenderData& render_data)
{
	std::lock_guard<std::mutex> lock(render_data_mutex_);

	if (!render_data_dirty_)
	{
		return false;
	}

	render_data = render_data_;
	render_data_dirty_ = false;
	return true;
}
