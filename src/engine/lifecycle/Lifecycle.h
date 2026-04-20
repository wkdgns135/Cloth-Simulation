#pragma once

class Lifecycle
{
public:
	virtual ~Lifecycle() = default;

	virtual void awake() {}
	virtual void start() {}
	virtual void update(float delta_time) {}
	virtual void stop() {}
	virtual void destroy() {}
};
