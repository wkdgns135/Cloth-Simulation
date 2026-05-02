#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "engine/core/Property.h"

using ObjectId = std::uint64_t;
constexpr ObjectId kInvalidObjectId = 0;

class Object
{
public:
	Object();
	explicit Object(std::string display_name);
	virtual ~Object() = default;

	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;

	virtual void awake() {}
	virtual void start() {}
	virtual void update(float delta_time) { static_cast<void>(delta_time); }
	virtual void stop() {}
	virtual void destroy() {}

	ObjectId id() const { return id_; }
	const std::string& display_name() const { return display_name_; }
	void set_display_name(std::string display_name);

	const std::vector<PropertyBase*>& properties() const { return properties_; }
	virtual std::optional<PropertyValue> get_property(std::string_view property_id) const;
	virtual bool set_property(std::string_view property_id, const PropertyValue& value);

	template <typename T>
	std::optional<T> get_typed_property(std::string_view property_id) const
	{
		const std::optional<PropertyValue> property_value = get_property(property_id);
		if (!property_value)
		{
			return std::nullopt;
		}

		T typed_value{};
		if (!convert_property_value(*property_value, typed_value))
		{
			return std::nullopt;
		}

		return typed_value;
	}

protected:
	virtual void on_property_changed(const PropertyBase& property);

private:
	friend class PropertyBase;

	void register_property(PropertyBase& property);
	PropertyBase* find_property(std::string_view property_id);
	const PropertyBase* find_property(std::string_view property_id) const;

	ObjectId id_ = kInvalidObjectId;
	std::string display_name_;
	std::vector<PropertyBase*> properties_;
};
