#include "engine/objects/CameraObject.h"

CameraObject::CameraObject()
{
	set_display_name("Camera");
}

Camera CameraObject::build_camera() const
{
	Camera camera;
	camera.position = get_object_world_position();
	camera.target = get_object_world_position() + get_object_forward_direction();
	camera.up = get_object_up_direction();
	camera.fov_y_degrees = fov_y_degrees();
	camera.near_plane = near_plane();
	camera.far_plane = far_plane();
	return camera;
}
