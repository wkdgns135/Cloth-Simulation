#pragma once

#include <algorithm>

#include "engine/core/WorldObject.h"
#include "engine/render/RenderScene.h"

class CameraObject : public WorldObject
{
public:
	CameraObject();

	PROPERTY_RANGE_NORMALIZED(float, fov_y_degrees, "Camera", "Field of View", 45.0f, 1.0f, 179.0f, 1.0f, [](float value) { return std::clamp(value, 1.0f, 179.0f); })
	PROPERTY_RANGE_NORMALIZED(float, near_plane, "Camera", "Near Plane", 0.01f, 0.001f, 10.0f, 0.001f, [](float value) { return std::max(value, 0.001f); })
	PROPERTY_RANGE_NORMALIZED(float, far_plane, "Camera", "Far Plane", 100.0f, 0.1f, 1000.0f, 0.1f, [](float value) { return std::max(value, 0.1f); })

	Camera build_camera() const;
};
