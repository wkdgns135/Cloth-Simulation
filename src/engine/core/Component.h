#pragma once

#include "engine/core/Object.h"

class WorldObject;

class Component : public Object
{
public:
	~Component() override = default;

	void set_owner(WorldObject* owner) { owner_ = owner; }
	WorldObject* owner() { return owner_; }
	const WorldObject* owner() const { return owner_; }

protected:
	void on_property_changed(const PropertyBase& property) override;

private:
	WorldObject* owner_ = nullptr;
};
