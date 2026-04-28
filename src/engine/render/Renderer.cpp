#include "engine/render/Renderer.h"

#include <QDebug>
#include <QOpenGLShader>
#include <cstddef>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer()
	: vbo_(QOpenGLBuffer::VertexBuffer)
	, ebo_(QOpenGLBuffer::IndexBuffer)
{
}

Renderer::~Renderer()
{
	vao_.destroy();
	vbo_.destroy();
	ebo_.destroy();
	program_.removeAllShaders();
}

void Renderer::initialize()
{
	initializeOpenGLFunctions();

	glEnable(GL_DEPTH_TEST);

	setup_shaders();

	vao_.create();
	vao_.bind();

	vbo_.create();
	vbo_.bind();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<void*>(offsetof(RenderVertex, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<void*>(offsetof(RenderVertex, normal)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<void*>(offsetof(RenderVertex, color)));
	glEnableVertexAttribArray(2);

	ebo_.create();

	vbo_.release();
	vao_.release();
}

void Renderer::resize(int width, int height)
{
	width_ = width > 0 ? width : 1;
	height_ = height > 0 ? height : 1;
	glViewport(0, 0, width_, height_);
}

void Renderer::render(const RenderScene& scene)
{
	glClearColor(0.12f, 0.12f, 0.16f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (scene.objects.empty())
	{
		return;
	}

	const float aspect = static_cast<float>(width_) / static_cast<float>(height_);
	const glm::mat4 view = glm::lookAt(scene.camera.position, scene.camera.target, scene.camera.up);
	const glm::mat4 projection = glm::perspective(
		glm::radians(scene.camera.fov_y_degrees),
		aspect,
		scene.camera.near_plane,
		scene.camera.far_plane);
	const glm::mat4 view_projection = projection * view;

	program_.bind();
	vao_.bind();

	for (const RenderObject& object : scene.objects)
	{
		draw_object(object, scene, view_projection);
	}

	vao_.release();
	program_.release();
}

void Renderer::setup_shaders()
{
	const char* vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec3 a_pos;
        layout (location = 1) in vec3 a_normal;
        layout (location = 2) in vec4 a_color;

        uniform mat4 u_model;
        uniform mat4 u_view_projection;
        uniform mat3 u_normal_matrix;

        out vec3 v_normal;
        out vec4 v_color;

        void main()
        {
            vec4 world_position = u_model * vec4(a_pos, 1.0);
            gl_Position = u_view_projection * world_position;
            v_normal = normalize(u_normal_matrix * a_normal);
            v_color = a_color;
        }
    )";

	const char* fragment_shader_source = R"(
        #version 330 core
        in vec3 v_normal;
        in vec4 v_color;

        uniform vec3 u_light_direction;
        uniform vec3 u_light_color;
        uniform float u_ambient_strength;
        uniform float u_diffuse_strength;
        uniform vec4 u_material_color;

        out vec4 FragColor;

        void main()
        {
            vec3 normal = normalize(v_normal);
            if (!gl_FrontFacing)
            {
                normal = -normal;
            }

            vec3 light_dir = normalize(u_light_direction);
            float diffuse = max(dot(normal, light_dir), 0.0);
            float lighting = u_ambient_strength + diffuse * u_diffuse_strength;

            vec4 base_color = v_color * u_material_color;
            FragColor = vec4(base_color.rgb * u_light_color * lighting, base_color.a);
        }
    )";

	if (!program_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source))
	{
		qDebug() << "Vertex shader compile failed:" << program_.log();
	}

	if (!program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source))
	{
		qDebug() << "Fragment shader compile failed:" << program_.log();
	}

	if (!program_.link())
	{
		qDebug() << "Shader program link failed:" << program_.log();
	}
}

void Renderer::draw_object(const RenderObject& object, const RenderScene& scene, const glm::mat4& view_projection)
{
	if (object.mesh.vertices.empty() || object.mesh.indices.empty())
	{
		return;
	}

	vbo_.bind();
	vbo_.allocate(
		object.mesh.vertices.data(),
		static_cast<int>(object.mesh.vertices.size() * sizeof(RenderVertex)));

	ebo_.bind();
	ebo_.allocate(
		object.mesh.indices.data(),
		static_cast<int>(object.mesh.indices.size() * sizeof(unsigned int)));

	const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(object.transform));
	const glm::vec3 light_direction = glm::normalize(scene.directional_light.direction);

	glUniformMatrix4fv(program_.uniformLocation("u_model"), 1, GL_FALSE, glm::value_ptr(object.transform));
	glUniformMatrix4fv(program_.uniformLocation("u_view_projection"), 1, GL_FALSE, glm::value_ptr(view_projection));
	glUniformMatrix3fv(program_.uniformLocation("u_normal_matrix"), 1, GL_FALSE, glm::value_ptr(normal_matrix));
	glUniform3fv(program_.uniformLocation("u_light_direction"), 1, glm::value_ptr(light_direction));
	glUniform3fv(program_.uniformLocation("u_light_color"), 1, glm::value_ptr(scene.directional_light.color));
	glUniform1f(program_.uniformLocation("u_ambient_strength"), scene.directional_light.ambient_strength);
	glUniform1f(program_.uniformLocation("u_diffuse_strength"), scene.directional_light.diffuse_strength);
	glUniform4fv(program_.uniformLocation("u_material_color"), 1, glm::value_ptr(object.material.base_color));

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(object.mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
}
