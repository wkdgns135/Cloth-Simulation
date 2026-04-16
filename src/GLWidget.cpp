#include "GLWidget.h"

#include <QOpenGLShader>
#include <QDebug>

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer)
{
}

GLWidget::~GLWidget()
{
    makeCurrent();

    vao.destroy();
    vbo.destroy();
    program.removeAllShaders();

    doneCurrent();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    setupShaders();
    setupGeometry();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    glClearColor(0.12f, 0.12f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program.bind();
    vao.bind();

    glDrawArrays(GL_POINTS, 0, 1);

    vao.release();
    program.release();
}

void GLWidget::setupShaders()
{
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = vec4(aPos, 1.0);
            gl_PointSize = 20.0;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(0.9, 0.2, 0.2, 1.0);
        }
    )";

    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource))
    {
        qDebug() << "Vertex shader compile failed:" << program.log();
    }

    if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource))
    {
        qDebug() << "Fragment shader compile failed:" << program.log();
    }

    if (!program.link())
    {
        qDebug() << "Shader program link failed:" << program.log();
    }
}

void GLWidget::setupGeometry()
{
    const float vertices[] = {
         0.0f, 0.0f, 0.0f
    };

    vao.create();
    vao.bind();

    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    vbo.release();
    vao.release();
}