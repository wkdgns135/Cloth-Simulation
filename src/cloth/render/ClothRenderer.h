#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>
#include <glm/glm.hpp>

class ClothRenderer : protected QOpenGLFunctions
{
public:
	ClothRenderer();
	~ClothRenderer();

	void initialize();
	void resize(int w, int h);
	void render();

	void set_particles(const std::vector<glm::vec3>& positions);

private:
	void setup_shaders();

private:
	QOpenGLShaderProgram program_;
	QOpenGLBuffer vbo_;
	QOpenGLVertexArrayObject vao_;

	std::vector<glm::vec3> positions_;
};
