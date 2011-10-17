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

    // TEST
    test_box = new CanvasBox(0, "test", PatchCanvas::ICON_APPLICATION, theme, scene);
}

CanvasTestApp::~CanvasTestApp()
{
    delete scene;
    delete theme;
    delete ui;
}
