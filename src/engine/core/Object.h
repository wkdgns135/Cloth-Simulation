#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

using ObjectId = std::uint64_t;
constexpr ObjectId kInvalidObjectId = 0;

enum class PropertyType
{
	Bool,
	Int,
	Float,
	Vec3,
	String,
};

using PropertyValue = std::variant<bool, int, float, glm::vec3, std::string>;

struct PropertyDesc
{
	std::string id;
	std::string label;
	std::string group;
	PropertyType type = PropertyType::Float;
	bool editable = true;
};

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

	virtual std::vector<PropertyDesc> property_descriptors() const;
	virtual std::optional<PropertyValue> get_property(std::string_view property_id) const;
	virtual bool set_property(std::string_view property_id, const PropertyValue& value);

private:
	ObjectId id_ = kInvalidObjectId;
	std::string display_name_;
};
