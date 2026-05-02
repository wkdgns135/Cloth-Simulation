#include "engine/core/Object.h"

#include <atomic>
#include <utility>

namespace
{
std::atomic<ObjectId> g_next_object_id{ 1 };
}

Object::Object()
	: id_(g_next_object_id.fetch_add(1, std::memory_order_relaxed))
{
}

Object::Object(std::string display_name)
	: Object()
{
	display_name_ = std::move(display_name);
}

void Object::set_display_name(std::string display_name)
{
	display_name_ = std::move(display_name);
}

void Object::on_property_changed(const PropertyBase& property)
{
	static_cast<void>(property);
}

void Object::register_property(PropertyBase& property)
{
	properties_.push_back(&property);
}

std::optional<PropertyValue> Object::get_property(std::string_view property_id) const
{
	if (const PropertyBase* property = find_property(property_id))
	{
		return property->value();
	}

	return std::nullopt;
}

bool Object::set_property(std::string_view property_id, const PropertyValue& value)
{
	if (PropertyBase* property = find_property(property_id))
	{
		return property->set_value(value);
	}

	return false;
}

PropertyBase* Object::find_property(std::string_view property_id)
{
	for (PropertyBase* property : properties_)
	{
		if (property && property->id() == property_id)
		{
			return property;
		}
	}

	return nullptr;
}

const PropertyBase* Object::find_property(std::string_view property_id) const
{
	for (const PropertyBase* property : properties_)
	{
		if (property && property->id() == property_id)
		{
			return property;
		}
	}

	return nullptr;
}
