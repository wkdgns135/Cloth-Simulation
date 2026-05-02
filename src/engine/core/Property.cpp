#include "engine/core/Property.h"

#include <utility>

#include "engine/core/Object.h"

PropertyBase::PropertyBase(Object& owner, PropertyDesc descriptor)
	: owner_(&owner)
	, descriptor_(std::move(descriptor))
{
	owner.register_property(*this);
}

void PropertyBase::notify_value_changed() const
{
	if (owner_)
	{
		owner_->on_property_changed(*this);
	}
}
