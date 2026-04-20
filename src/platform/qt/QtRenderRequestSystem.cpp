#include "platform/qt/QtRenderRequestSystem.h"

#include <QMetaObject>

#include "platform/qt/MainWindow.h"

QtRenderRequestSystem::QtRenderRequestSystem(MainWindow& window)
	: window_(window)
{
}

void QtRenderRequestSystem::update(float)
{
	MainWindow* window = &window_;

	QMetaObject::invokeMethod(window, [window]() {
		window->request_render();
	}, Qt::QueuedConnection);
}
