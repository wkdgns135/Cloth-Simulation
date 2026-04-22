#include <QApplication>
#include <QMetaObject>

#include "cloth/ClothWorld.h"
#include "engine/Engine.h"
#include "engine/render/RenderSystem.h"
#include "platform/qt/MainWindow.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	ClothWorld cloth_world;
	RenderSystem render_system;
	Engine engine(cloth_world, render_system);

	MainWindow window(render_system, engine);
	engine.set_render_request_callback([&window]() {
		QMetaObject::invokeMethod(&window, [&window]() {
			window.request_render();
		}, Qt::QueuedConnection);
	});

	engine.initialize();
	window.show();
	engine.start();

	const int result = app.exec();

	engine.shutdown();

	return result;
}
