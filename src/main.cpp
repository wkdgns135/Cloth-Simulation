#include <QApplication>

#include "engine/Engine.h"
#include "cloth/ClothWorld.h"
#include "cloth/physics/ClothPhysicsSystem.h"
#include "cloth/render/ClothRenderSystem.h"
#include "cloth/simulation/ClothSimulationSystem.h"
#include "platform/qt/MainWindow.h"
#include "platform/qt/QtRenderRequestSystem.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	ClothWorld cloth_world;
	ClothSimulationSystem simulation_system(cloth_world);
	ClothPhysicsSystem physics_system(cloth_world);
	ClothRenderSystem render_system(cloth_world);

	MainWindow window(render_system);
	QtRenderRequestSystem render_request_system(window);

	Engine engine;
	engine.add_simulation_system(simulation_system);
	engine.add_physics_system(physics_system);
	engine.add_render_system(render_request_system);

	engine.initialize();
	window.show();
	engine.start();

	const int result = app.exec();

	engine.shutdown();

	return result;
}
