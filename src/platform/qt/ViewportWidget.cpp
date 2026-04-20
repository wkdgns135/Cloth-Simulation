#include "platform/qt/ViewportWidget.h"

#include "cloth/render/ClothRenderSystem.h"

ViewportWidget::ViewportWidget(ClothRenderSystem& render_system, QWidget* parent)
	: QOpenGLWidget(parent)
	, render_system_(render_system)
{
}

ViewportWidget::~ViewportWidget()
{
	if (context())
	{
		makeCurrent();
		render_system_.shutdown_gl();
		doneCurrent();
		return;
	}

	render_system_.shutdown_gl();
}

void ViewportWidget::initializeGL()
{
	initializeOpenGLFunctions();
	render_system_.initialize_gl();
}

void ViewportWidget::resizeGL(int w, int h)
{
	render_system_.resize(w, h);
}

void ViewportWidget::paintGL()
{
	render_system_.render();
}
