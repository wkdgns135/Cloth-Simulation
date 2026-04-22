#include "cloth/ClothWorld.h"

#include "cloth/ClothObject.h"
#include "components/PBDClothSimulationComponent.h"
#include "components/XPBDClothSimulationComponent.h"

ClothWorld::ClothWorld()
{
	camera().position = glm::vec3(0.0f, 0.0f, 2.6f);
	camera().target = glm::vec3(0.0f, 0.0f, 0.0f);
	camera().fov_y_degrees = 45.0f;

	directional_light().direction = glm::vec3(-0.75f, 0.65f, 0.45f);
	directional_light().ambient_strength = 0.16f;
	directional_light().diffuse_strength = 1.15f;

	ClothObject& pbd_cloth = create_object<ClothObject>(100, 100, 0.05f);
	pbd_cloth.add_component<PBDClothSimulationComponent>(pbd_cloth.cloth());
	pbd_cloth.transform().position = glm::vec3(-3.f, 0.0f, 0.0f);

	ClothObject& xpbd_cloth = create_object<ClothObject>(100, 100, 0.05f);
	xpbd_cloth.add_component<XPBDClothSimulationComponent>(xpbd_cloth.cloth());
	xpbd_cloth.transform().position = glm::vec3(3.f, 0.0f, 0.0f);
}
