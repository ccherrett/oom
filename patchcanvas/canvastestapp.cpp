#include "canvastestapp.h"
#include "ui_canvastestapp.h"

CanvasTestApp::CanvasTestApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CanvasTestApp)
{
    ui->setupUi(this);

    theme = new Theme();

    scene = new PatchScene();
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);
    ui->graphicsView->setRenderHint(QPainter::TextAntialiasing, true);

    scene->rubberbandByTheme(theme);
}

CanvasTestApp::~CanvasTestApp()
{
    delete scene;
    delete theme;
    delete ui;
}
