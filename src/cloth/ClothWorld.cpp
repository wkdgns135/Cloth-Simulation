#include "cloth/ClothWorld.h"

#include <glm/geometric.hpp>

#include "cloth/ClothObject.h"
#include "cloth/components/ClothCameraInputComponent.h"
#include "components/PBDClothSimulationComponent.h"
#include "components/XPBDClothSimulationComponent.h"
#include "engine/objects/CameraObject.h"
#include "engine/objects/DirectionalLightObject.h"

ClothWorld::ClothWorld()
{
	main_camera_object_->transform().position = glm::vec3(0.0f, 0.0f, 2.6f);
	main_camera_object_->transform().look_at(glm::vec3(0.0f, 0.0f, 0.0f));
	main_camera_object_->fov_y_degrees = 45.0f;
	main_camera_object_->add_component<ClothCameraInputComponent>();

	main_directional_light_object_->transform().set_forward(glm::normalize(glm::vec3(-0.75f, 0.65f, 0.45f)));
	main_directional_light_object_->ambient_strength = 0.16f;
	main_directional_light_object_->diffuse_strength = 1.15f;

	// ClothObject& pbd_cloth = create_object<ClothObject>(64, 64, 0.05f);
	// pbd_cloth.add_component<PBDClothSimulationComponent>(pbd_cloth.cloth());
	// pbd_cloth.transform().position = glm::vec3(-3.f, 0.0f, 0.0f);

	ClothObject& xpbd_cloth = create_object<ClothObject>("asset/test_cloth_patch.obj");
	xpbd_cloth.add_component<PBDClothSimulationComponent>(xpbd_cloth.cloth());
	xpbd_cloth.transform().position = glm::vec3(0.0f, 0.0f, 0.0f);
}
