#pragma once

#include <mutex>
#include <vector>

#include "cloth/core/Cloth.h"
#include "cloth/physics/ClothSimulator.h"
#include "cloth/render/ClothRenderData.h"

class ClothWorld final
{
public:
	void initialize();
	void step_physics(ClothSimulator& simulator, float delta_time);
	void publish_render_data();
	bool consume_render_data(ClothRenderData& render_data);
	const std::vector<unsigned int>* get_indices();

private:
	std::mutex cloth_mutex_;
	std::mutex render_data_mutex_;

	Cloth cloth_;
	ClothRenderData render_data_;
	bool render_data_dirty_ = false;
	bool initialized_ = false;
};
