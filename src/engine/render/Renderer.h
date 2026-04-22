#pragma once

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include "engine/render/RenderScene.h"

class Renderer : protected QOpenGLFunctions
{
public:
	Renderer();
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	void initialize();
	void resize(int width, int height);
	void render(const RenderScene& scene);

private:
	void setup_shaders();
	void draw_object(const RenderObject& object, const RenderScene& scene, const glm::mat4& view_projection);

	QOpenGLShaderProgram program_;
	QOpenGLBuffer vbo_;
	QOpenGLBuffer ebo_;
	QOpenGLVertexArrayObject vao_;

	int width_ = 1;
	int height_ = 1;
};
