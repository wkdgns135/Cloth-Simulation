#include "engine/core/Component.h"

#include "engine/core/WorldObject.h"

void Component::on_property_changed(const PropertyBase& property)
{
	if (owner_)
	{
		owner_->on_component_property_changed(*this, property);
		return;
	}

	Object::on_property_changed(property);
}
