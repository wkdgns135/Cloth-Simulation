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

std::vector<PropertyDesc> Object::property_descriptors() const
{
	return {};
}

std::optional<PropertyValue> Object::get_property(std::string_view property_id) const
{
	static_cast<void>(property_id);
	return std::nullopt;
}

bool Object::set_property(std::string_view property_id, const PropertyValue& value)
{
	static_cast<void>(property_id);
	static_cast<void>(value);
	return false;
}
