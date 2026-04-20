#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class ClothRenderSystem;

class ViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	explicit ViewportWidget(ClothRenderSystem& render_system, QWidget* parent = nullptr);
	~ViewportWidget() override;

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

private:
	ClothRenderSystem& render_system_;
};
