#pragma once

#include <filesystem>

#include "cloth/core/Cloth.h"
#include "engine/core/Object.h"

class ClothObject final : public Object
{
public:
	ClothObject(int width, int height, float spacing);
	explicit ClothObject(const std::filesystem::path& mesh_path);

	Cloth& cloth() { return cloth_; }
	const Cloth& cloth() const { return cloth_; }

private:
	Cloth cloth_;
};
