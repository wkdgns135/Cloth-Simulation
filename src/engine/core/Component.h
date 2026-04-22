#pragma once

#include "engine/lifecycle/Lifecycle.h"

class Object;

class Component : public Lifecycle
{
public:
	~Component() override = default;

	void set_owner(Object* owner) { owner_ = owner; }
	Object* owner() { return owner_; }
	const Object* owner() const { return owner_; }

private:
	Object* owner_ = nullptr;
};
