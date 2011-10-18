#include "canvastestapp.h"
#include "ui_canvastestapp.h"

#include <cstdio>

CanvasTestApp::CanvasTestApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CanvasTestApp)
{
    ui->setupUi(this);

    scene = new PatchCanvas::PatchScene();
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);
    ui->graphicsView->setRenderHint(QPainter::TextAntialiasing, true);

    PatchCanvas::init(scene, 0, true);

    scene->rubberbandByTheme();

    // TEST
    PatchCanvas::addGroup(0, "group1");
    PatchCanvas::addGroup(1, "group2");
}

CanvasTestApp::~CanvasTestApp()
{
    PatchCanvas::clear();

    delete scene;
    delete ui;
}
