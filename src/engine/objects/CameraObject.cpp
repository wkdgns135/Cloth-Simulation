#include "engine/objects/CameraObject.h"

Camera CameraObject::build_camera() const
{
	Camera camera;
	camera.position = transform().position;
	camera.target = transform().position + transform().forward();
	camera.up = transform().up();
	camera.fov_y_degrees = fov_y_degrees;
	camera.near_plane = near_plane;
	camera.far_plane = far_plane;
	return camera;
}
