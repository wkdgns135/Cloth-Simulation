#include "engine/objects/DirectionalLightObject.h"

DirectionalLight DirectionalLightObject::build_light() const
{
	DirectionalLight light;
	light.direction = transform().forward();
	light.color = color;
	light.ambient_strength = ambient_strength;
	light.diffuse_strength = diffuse_strength;
	return light;
}
