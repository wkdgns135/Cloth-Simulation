#include "engine/objects/DirectionalLightObject.h"

DirectionalLightObject::DirectionalLightObject()
{
	set_display_name("Directional Light");
}

DirectionalLight DirectionalLightObject::build_light() const
{
	DirectionalLight light;
	light.direction = get_object_forward_direction();
	light.color = color();
	light.ambient_strength = ambient_strength();
	light.diffuse_strength = diffuse_strength();
	return light;
}
