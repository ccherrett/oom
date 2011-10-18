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
    PatchCanvas::addGroup(0, "Box with timer -> 0");

    testTimerCount = 0;
    testTimer = startTimer(100);
}

CanvasTestApp::~CanvasTestApp()
{
    PatchCanvas::clear();

    delete scene;
    delete ui;
}

void CanvasTestApp::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == testTimer)
    {
        testTimerCount++;
        PatchCanvas::renameGroup(0, QString("Box with timer -> %1").arg(testTimerCount).toStdString().data());
    }
    QMainWindow::timerEvent(event);
}
