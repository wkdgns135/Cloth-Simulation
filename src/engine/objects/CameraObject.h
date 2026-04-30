#pragma once

#include "engine/core/WorldObject.h"
#include "engine/render/RenderScene.h"

class CameraObject : public WorldObject
{
public:
	float fov_y_degrees = 45.0f;
	float near_plane = 0.01f;
	float far_plane = 100.0f;

	Camera build_camera() const;
};
