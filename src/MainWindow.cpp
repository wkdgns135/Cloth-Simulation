#include "MainWindow.h"
#include "GLWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    resize(1280, 720);
    setWindowTitle("PBD Cloth Simulator");

    glWidget = new GLWidget(this);
    setCentralWidget(glWidget);
}