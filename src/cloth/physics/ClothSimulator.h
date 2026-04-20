#pragma once

#include <glm/glm.hpp>

class Cloth;
struct Particle;

class ClothSimulator
{
public:
	void step(Cloth& cloth, float delta_time) const;

	void set_gravity(const glm::vec3& gravity) { gravity_ = gravity; }
	void set_damping(float damping) { damping_ = damping; }
	void set_constraint_iterations(int iterations) { constraint_iterations_ = iterations; }

private:
	void integrate(Cloth& cloth, float delta_time) const;
	void solve_structural_constraints(Cloth& cloth) const;
	void satisfy_distance(Particle& a, Particle& b, float rest_length) const;

	glm::vec3 gravity_ = glm::vec3(0.0f, -9.8f, 0.0f);
	float damping_ = 0.99f;
	int constraint_iterations_ = 6;
};
