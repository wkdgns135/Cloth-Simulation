#pragma once

#include <vector>

#include <glm/glm.hpp>

struct Camera
{
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 2.0f);
	glm::vec3 target = glm::vec3(0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	float fov_y_degrees = 45.0f;
	float near_plane = 0.01f;
	float far_plane = 100.0f;
};

struct DirectionalLight
{
	glm::vec3 direction = glm::vec3(-0.35f, 0.65f, 0.70f);
	glm::vec3 color = glm::vec3(1.0f);
	float ambient_strength = 0.28f;
	float diffuse_strength = 0.72f;
};

struct RenderVertex
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0f);
};

struct MeshData
{
	std::vector<RenderVertex> vertices;
	std::vector<unsigned int> indices;
};

struct Material
{
	glm::vec4 base_color = glm::vec4(1.0f);
};

struct RenderObject
{
	MeshData mesh;
	glm::mat4 transform = glm::mat4(1.0f);
	Material material;
};

struct RenderScene
{
	std::vector<RenderObject> objects;
	Camera camera;
	DirectionalLight directional_light;
};
