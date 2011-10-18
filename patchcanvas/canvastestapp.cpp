#include "canvastestapp.h"
#include "ui_canvastestapp.h"

#include <cstdio>

CanvasTestApp::CanvasTestApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CanvasTestApp)
{
    ui->setupUi(this);

    canvas = new PatchCanvas::Canvas;

    scene = new PatchScene();
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);
    ui->graphicsView->setRenderHint(QPainter::TextAntialiasing, true);

    canvas->init(scene, 0, true);

    scene->rubberbandByTheme(canvas->theme);

    // TEST
    printf("%s\n", canvas->theme->name.toStdString().data());
    box1 = new CanvasBox(0, "box with long name on purpose", PatchCanvas::ICON_APPLICATION, scene, canvas);
    box2 = new CanvasBox(1, "harware box", PatchCanvas::ICON_HARDWARE, scene, canvas);
    box3 = new CanvasBox(2, "mplayer box", PatchCanvas::ICON_APPLICATION, scene, canvas);
}

CanvasTestApp::~CanvasTestApp()
{
    delete box1;
    delete box2;
    delete box3;
    delete scene;
    delete canvas;
    delete ui;
}
