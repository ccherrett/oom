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

    PatchCanvas::options_t options;
    options.antialiasing = Qt::Checked;
    options.auto_hide_groups = false;
    options.bezier_lines = false;
    options.connect_midi2outro = false;
    options.fancy_eyecandy = false;
    options.theme_name = Theme::getThemeName(Theme::getDefaultTheme());

    PatchCanvas::init(scene, 0, true);

    scene->rubberbandByTheme();

    // TEST
    PatchCanvas::addGroup(0, "Box with timer [split] -> 0", true);
    PatchCanvas::addPort(0, 0, "AudioInputPort", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_AUDIO);
    PatchCanvas::addPort(0, 1, "MidiInputPort", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_MIDI);
    PatchCanvas::addPort(0, 2, "OutroInputPort", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_OUTRO);
    PatchCanvas::addPort(0, 3, "AudioOutputPort", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_AUDIO);
    PatchCanvas::addPort(0, 4, "MidiOutputPort", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_MIDI);
    PatchCanvas::addPort(0, 5, "OutroOutputPort", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_OUTRO);

    PatchCanvas::addGroup(1, "Simple box", false);
    PatchCanvas::addPort(1, 6, "Some Random Port 1", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_OUTRO);
    PatchCanvas::addPort(1, 7, "Some Random Port 2", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_OUTRO);
    PatchCanvas::addPort(1, 8, "An input", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_AUDIO);

    PatchCanvas::connectPorts(0, 3, 8);

    testTimerCount = 0;
    testTimer = startTimer(300);
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
        testTimerCount += 100;
        PatchCanvas::renameGroup(0, QString("Box with timer -> %1").arg(testTimerCount).toStdString().data());
    }
    QMainWindow::timerEvent(event);
}
