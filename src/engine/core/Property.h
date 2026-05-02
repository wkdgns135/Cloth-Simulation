#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <glm/glm.hpp>

class Object;

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
	std::optional<PropertyValue> min_value;
	std::optional<PropertyValue> max_value;
	std::optional<PropertyValue> step;
};

template <typename T>
struct PropertyTypeResolver;

template <>
struct PropertyTypeResolver<bool>
{
	static constexpr PropertyType value = PropertyType::Bool;
};

template <>
struct PropertyTypeResolver<int>
{
	static constexpr PropertyType value = PropertyType::Int;
};

template <>
struct PropertyTypeResolver<float>
{
	static constexpr PropertyType value = PropertyType::Float;
};

template <>
struct PropertyTypeResolver<glm::vec3>
{
	static constexpr PropertyType value = PropertyType::Vec3;
};

template <>
struct PropertyTypeResolver<std::string>
{
	static constexpr PropertyType value = PropertyType::String;
};

template <typename T>
constexpr PropertyType property_type_v = PropertyTypeResolver<std::decay_t<T>>::value;

template <typename T>
using PropertyGetterResult =
	std::conditional_t<std::is_arithmetic_v<std::decay_t<T>> || std::is_enum_v<std::decay_t<T>>,
		std::decay_t<T>,
		const std::decay_t<T>&>;

template <typename T>
bool convert_property_value(const PropertyValue& value, T& out)
{
	if (const auto* typed_value = std::get_if<std::decay_t<T>>(&value))
	{
		out = *typed_value;
		return true;
	}

	return false;
}

template <>
inline bool convert_property_value<float>(const PropertyValue& value, float& out)
{
	if (const auto* typed_value = std::get_if<float>(&value))
	{
		out = *typed_value;
		return true;
	}

	if (const auto* typed_value = std::get_if<int>(&value))
	{
		out = static_cast<float>(*typed_value);
		return true;
	}

	return false;
}

class PropertyBase
{
public:
	PropertyBase(Object& owner, PropertyDesc descriptor);

	virtual ~PropertyBase() = default;

	const PropertyDesc& descriptor() const { return descriptor_; }
	std::string_view id() const { return descriptor_.id; }
	bool editable() const { return descriptor_.editable; }

	virtual PropertyValue value() const = 0;
	virtual bool set_value(const PropertyValue& value) = 0;

protected:
	void notify_value_changed() const;

private:
	Object* owner_ = nullptr;
	PropertyDesc descriptor_;
};

template <typename T>
struct PropertyConfig
{
	std::string id;
	std::string label;
	std::string group;
	bool editable = true;
	std::optional<T> min_value;
	std::optional<T> max_value;
	std::optional<T> step;
};

template <typename T>
PropertyConfig<T> make_property_config(
	std::string_view id,
	std::string_view label,
	std::string_view group,
	bool editable = true)
{
	PropertyConfig<T> config;
	config.id = id;
	config.label = label;
	config.group = group;
	config.editable = editable;
	return config;
}

template <typename T>
PropertyConfig<T> make_ranged_property_config(
	std::string_view id,
	std::string_view label,
	std::string_view group,
	T min_value,
	T max_value,
	T step,
	bool editable = true)
{
	PropertyConfig<T> config = make_property_config<T>(id, label, group, editable);
	config.min_value = min_value;
	config.max_value = max_value;
	config.step = step;
	return config;
}

template <typename T>
class Property : public PropertyBase
{
public:
	using ValueType = T;
	using Normalizer = std::function<T(T)>;
	using ChangeHandler = std::function<void(const T&)>;

	Property(Object& owner, PropertyConfig<T> config, T initial_value, Normalizer normalizer = {}, ChangeHandler on_changed = {})
		: PropertyBase(owner, make_descriptor(config))
		, value_(normalize_value(std::move(initial_value), normalizer))
		, normalizer_(std::move(normalizer))
		, on_changed_(std::move(on_changed))
	{
	}

	const T& get() const { return value_; }

	void set(T value)
	{
		value_ = normalize_value(std::move(value), normalizer_);
		if (on_changed_)
		{
			on_changed_(value_);
		}
		notify_value_changed();
	}

	PropertyGetterResult<T> operator()() const
	{
		return get();
	}

	void operator()(T value)
	{
		set(std::move(value));
	}

	Property& operator=(T value)
	{
		set(std::move(value));
		return *this;
	}

	PropertyValue value() const override
	{
		return value_;
	}

	bool set_value(const PropertyValue& value) override
	{
		if (!editable())
		{
			return false;
		}

		T typed_value{};
		if (!convert_property_value(value, typed_value))
		{
			return false;
		}

		set(std::move(typed_value));
		return true;
	}

private:
	static PropertyDesc make_descriptor(const PropertyConfig<T>& config)
	{
		PropertyDesc descriptor;
		descriptor.id = config.id;
		descriptor.label = config.label;
		descriptor.group = config.group;
		descriptor.type = property_type_v<T>;
		descriptor.editable = config.editable;

		if (config.min_value)
		{
			descriptor.min_value = PropertyValue(*config.min_value);
		}
		if (config.max_value)
		{
			descriptor.max_value = PropertyValue(*config.max_value);
		}
		if (config.step)
		{
			descriptor.step = PropertyValue(*config.step);
		}

		return descriptor;
	}

	static T normalize_value(T value, const Normalizer& normalizer)
	{
		if (normalizer)
		{
			return normalizer(std::move(value));
		}

		return value;
	}

	T value_;
	Normalizer normalizer_;
	ChangeHandler on_changed_;
};

#define PROPERTY_ACCESSORS(Type, Name) \
	void set_##Name(Type value) { Name##_(std::move(value)); } \
	PropertyGetterResult<Type> Name() const { return Name##_(); }

#define PROPERTY(Type, Name, Group, Label, InitialValue) \
	PROPERTY_ACCESSORS(Type, Name) \
	private: \
	Property<Type> Name##_{ *this, make_property_config<Type>(#Name, Label, Group), InitialValue }; \
	public:

#define PROPERTY_RANGE(Type, Name, Group, Label, InitialValue, MinValue, MaxValue, StepValue) \
	PROPERTY_ACCESSORS(Type, Name) \
	private: \
	Property<Type> Name##_{ \
		*this, \
		make_ranged_property_config<Type>(#Name, Label, Group, MinValue, MaxValue, StepValue), \
		InitialValue }; \
	public:

#define PROPERTY_NORMALIZED(Type, Name, Group, Label, InitialValue, NormalizerExpr) \
	PROPERTY_ACCESSORS(Type, Name) \
	private: \
	Property<Type> Name##_{ *this, make_property_config<Type>(#Name, Label, Group), InitialValue, NormalizerExpr }; \
	public:

#define PROPERTY_RANGE_NORMALIZED(Type, Name, Group, Label, InitialValue, MinValue, MaxValue, StepValue, NormalizerExpr) \
	PROPERTY_ACCESSORS(Type, Name) \
	private: \
	Property<Type> Name##_{ \
		*this, \
		make_ranged_property_config<Type>(#Name, Label, Group, MinValue, MaxValue, StepValue), \
		InitialValue, \
		NormalizerExpr }; \
	public:
