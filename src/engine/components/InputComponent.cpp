#include "engine/components/InputComponent.h"

#include "engine/core/Object.h"
#include "engine/core/World.h"

void InputComponent::awake()
{
	if (registered_with_world_)
	{
		return;
	}

	Object* object_owner = owner();
	if (!object_owner || !object_owner->world())
	{
		return;
	}

	World& world = *object_owner->world();

	for (const KeyBinding& binding : key_pressed_bindings_)
	{
		const InputSubscriptionHandle handle = world.subscribe_key_pressed(binding.key, binding.handler);
		if (handle.is_valid())
		{
			key_pressed_subscriptions_.push_back(KeySubscription{ binding.key, handle });
		}
	}

	for (const KeyBinding& binding : key_released_bindings_)
	{
		const InputSubscriptionHandle handle = world.subscribe_key_released(binding.key, binding.handler);
		if (handle.is_valid())
		{
			key_released_subscriptions_.push_back(KeySubscription{ binding.key, handle });
		}
	}

	for (const PointerBinding& binding : pointer_pressed_bindings_)
	{
		const InputSubscriptionHandle handle = world.subscribe_pointer_pressed(binding.button, binding.handler);
		if (handle.is_valid())
		{
			pointer_pressed_subscriptions_.push_back(PointerSubscription{ binding.button, handle });
		}
	}

	for (const PointerBinding& binding : pointer_released_bindings_)
	{
		const InputSubscriptionHandle handle = world.subscribe_pointer_released(binding.button, binding.handler);
		if (handle.is_valid())
		{
			pointer_released_subscriptions_.push_back(PointerSubscription{ binding.button, handle });
		}
	}

	for (const PointerHandler& handler : pointer_moved_handlers_)
	{
		const InputSubscriptionHandle handle = world.subscribe_pointer_moved(handler);
		if (handle.is_valid())
		{
			pointer_moved_subscriptions_.push_back(handle);
		}
	}

	for (const WheelHandler& handler : wheel_scrolled_handlers_)
	{
		const InputSubscriptionHandle handle = world.subscribe_wheel_scrolled(handler);
		if (handle.is_valid())
		{
			wheel_scrolled_subscriptions_.push_back(handle);
		}
	}

	registered_with_world_ = true;
}

void InputComponent::destroy()
{
	if (!registered_with_world_)
	{
		return;
	}

	Object* object_owner = owner();
	if (object_owner && object_owner->world())
	{
		World& world = *object_owner->world();

		for (const KeySubscription& subscription : key_pressed_subscriptions_)
		{
			world.unsubscribe_key_pressed(subscription.key, subscription.handle);
		}

		for (const KeySubscription& subscription : key_released_subscriptions_)
		{
			world.unsubscribe_key_released(subscription.key, subscription.handle);
		}

		for (const PointerSubscription& subscription : pointer_pressed_subscriptions_)
		{
			world.unsubscribe_pointer_pressed(subscription.button, subscription.handle);
		}

		for (const PointerSubscription& subscription : pointer_released_subscriptions_)
		{
			world.unsubscribe_pointer_released(subscription.button, subscription.handle);
		}

		for (const InputSubscriptionHandle handle : pointer_moved_subscriptions_)
		{
			world.unsubscribe_pointer_moved(handle);
		}

		for (const InputSubscriptionHandle handle : wheel_scrolled_subscriptions_)
		{
			world.unsubscribe_wheel_scrolled(handle);
		}
	}

	key_pressed_subscriptions_.clear();
	key_released_subscriptions_.clear();
	pointer_pressed_subscriptions_.clear();
	pointer_released_subscriptions_.clear();
	pointer_moved_subscriptions_.clear();
	wheel_scrolled_subscriptions_.clear();
	registered_with_world_ = false;
}

void InputComponent::bind_key_pressed(InputKey key, KeyHandler handler)
{
	if (key == InputKey::Unknown || !handler)
	{
		return;
	}

	key_pressed_bindings_.push_back(KeyBinding{ key, std::move(handler) });

	Object* object_owner = owner();
	if (registered_with_world_ && object_owner && object_owner->world())
	{
		const InputSubscriptionHandle handle
			= object_owner->world()->subscribe_key_pressed(key, key_pressed_bindings_.back().handler);
		if (handle.is_valid())
		{
			key_pressed_subscriptions_.push_back(KeySubscription{ key, handle });
		}
	}
}

void InputComponent::bind_key_released(InputKey key, KeyHandler handler)
{
	if (key == InputKey::Unknown || !handler)
	{
		return;
	}

	key_released_bindings_.push_back(KeyBinding{ key, std::move(handler) });

	Object* object_owner = owner();
	if (registered_with_world_ && object_owner && object_owner->world())
	{
		const InputSubscriptionHandle handle
			= object_owner->world()->subscribe_key_released(key, key_released_bindings_.back().handler);
		if (handle.is_valid())
		{
			key_released_subscriptions_.push_back(KeySubscription{ key, handle });
		}
	}
}

void InputComponent::bind_pointer_pressed(PointerButton button, PointerHandler handler)
{
	if (button == PointerButton::None || !handler)
	{
		return;
	}

	pointer_pressed_bindings_.push_back(PointerBinding{ button, std::move(handler) });

	Object* object_owner = owner();
	if (registered_with_world_ && object_owner && object_owner->world())
	{
		const InputSubscriptionHandle handle
			= object_owner->world()->subscribe_pointer_pressed(button, pointer_pressed_bindings_.back().handler);
		if (handle.is_valid())
		{
			pointer_pressed_subscriptions_.push_back(PointerSubscription{ button, handle });
		}
	}
}

void InputComponent::bind_pointer_released(PointerButton button, PointerHandler handler)
{
	if (button == PointerButton::None || !handler)
	{
		return;
	}

	pointer_released_bindings_.push_back(PointerBinding{ button, std::move(handler) });

	Object* object_owner = owner();
	if (registered_with_world_ && object_owner && object_owner->world())
	{
		const InputSubscriptionHandle handle
			= object_owner->world()->subscribe_pointer_released(button, pointer_released_bindings_.back().handler);
		if (handle.is_valid())
		{
			pointer_released_subscriptions_.push_back(PointerSubscription{ button, handle });
		}
	}
}

void InputComponent::bind_pointer_moved(PointerHandler handler)
{
	if (!handler)
	{
		return;
	}

	pointer_moved_handlers_.push_back(std::move(handler));

	Object* object_owner = owner();
	if (registered_with_world_ && object_owner && object_owner->world())
	{
		const InputSubscriptionHandle handle
			= object_owner->world()->subscribe_pointer_moved(pointer_moved_handlers_.back());
		if (handle.is_valid())
		{
			pointer_moved_subscriptions_.push_back(handle);
		}
	}
}

void InputComponent::bind_wheel_scrolled(WheelHandler handler)
{
	if (!handler)
	{
		return;
	}

	wheel_scrolled_handlers_.push_back(std::move(handler));

	Object* object_owner = owner();
	if (registered_with_world_ && object_owner && object_owner->world())
	{
		const InputSubscriptionHandle handle
			= object_owner->world()->subscribe_wheel_scrolled(wheel_scrolled_handlers_.back());
		if (handle.is_valid())
		{
			wheel_scrolled_subscriptions_.push_back(handle);
		}
	}
}
