#pragma once

#include <algorithm>

#include <glm/glm.hpp>

#include "engine/core/WorldObject.h"
#include "engine/render/RenderScene.h"

class DirectionalLightObject : public WorldObject
{
public:
	DirectionalLightObject();

	PROPERTY(glm::vec3, color, "Light", "Color", glm::vec3(1.0f))
	PROPERTY_RANGE_NORMALIZED(float, ambient_strength, "Light", "Ambient Strength", 0.28f, 0.0f, 4.0f, 0.01f, [](float value) { return std::max(value, 0.0f); })
	PROPERTY_RANGE_NORMALIZED(float, diffuse_strength, "Light", "Diffuse Strength", 0.72f, 0.0f, 4.0f, 0.01f, [](float value) { return std::max(value, 0.0f); })

	DirectionalLight build_light() const;
};
