#include "cloth/render/ClothRenderer.h"

#include <QOpenGLShader>
#include <QDebug>

ClothRenderer::ClothRenderer()
	: vbo_(QOpenGLBuffer::VertexBuffer)
	, ebo_(QOpenGLBuffer::IndexBuffer)
{
}

ClothRenderer::~ClothRenderer()
{
	vao_.destroy();
	vbo_.destroy();
	ebo_.destroy();
	program_.removeAllShaders();
}

void ClothRenderer::initialize()
{
	initializeOpenGLFunctions();

	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_DEPTH_TEST);

	setup_shaders();

	vao_.create();
	vao_.bind();

	vbo_.create();
	vbo_.bind();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
	glEnableVertexAttribArray(0);

	ebo_.create();

	vbo_.release();
	vao_.release();
}
void ClothRenderer::resize(int w, int h)
{
	glViewport(0, 0, w, h);
}

void ClothRenderer::render()
{
	glClearColor(0.12f, 0.12f, 0.16f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (positions_.empty()) return;
	if (index_count_ == 0) return;

	program_.bind();
	vao_.bind();

	glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);

	vao_.release();
	program_.release();
}

void ClothRenderer::setup_shaders()
{
	const char* vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec3 a_pos;

        void main()
        {
            gl_Position = vec4(a_pos, 1.0);
            gl_PointSize = 20.0;
        }
    )";

	const char* fragment_shader_source = R"(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(0.9, 0.2, 0.2, 1.0);
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

void ClothRenderer::set_particles(const std::vector<glm::vec3>& positions)
{
	positions_ = positions;

	if (!vao_.isCreated()) return;

	vao_.bind();
	vbo_.bind();

	vbo_.allocate(positions_.data(), static_cast<int>(positions_.size() * sizeof(glm::vec3)));

	vbo_.release();
	vao_.release();
}

void ClothRenderer::set_indices(const std::vector<unsigned int>& indices)
{
	if (!vao_.isCreated() || !ebo_.isCreated()) return;

	index_count_ = static_cast<GLsizei>(indices.size());

	vao_.bind();

	ebo_.bind();
	ebo_.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(unsigned int)));

	vao_.release();
}
