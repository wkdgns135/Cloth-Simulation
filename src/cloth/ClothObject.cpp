#include "cloth/ClothObject.h"

#include "cloth/components/ClothRenderComponent.h"

ClothObject::ClothObject(int width, int height, float spacing)
	: cloth_(width, height, spacing)
{
	// for (int i = 0; i < cloth_.get_width(); ++i)
	// {
	// 	cloth_.set_fixed(i, 0, true);
	// }
	cloth_.set_fixed(0, 0, true);
	cloth_.set_fixed(cloth_.get_width() - 1, 0, true);
	add_component<ClothRenderComponent>(cloth_);
}
