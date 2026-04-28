#pragma once

#include <glm/glm.hpp>

#include "engine/core/Object.h"
#include "engine/render/RenderScene.h"

class DirectionalLightObject : public Object
{
public:
	glm::vec3 color = glm::vec3(1.0f);
	float ambient_strength = 0.28f;
	float diffuse_strength = 0.72f;

	DirectionalLight build_light() const;
};
