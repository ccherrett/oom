#include "canvastestapp.h"
#include "ui_canvastestapp.h"

#include <cstdio>

static void canvas_callback(PatchCanvas::CallbackAction action, int value1, int value2, QString value_str)
{
    printf("--------------------------- Callback called %i|%i|%i|%s\n", action, value1, value2, value_str.toStdString().data());

    int group_id;

    switch (action)
    {
    case PatchCanvas::ACTION_PORTS_CONNECT:
        break;
    case PatchCanvas::ACTION_PORTS_DISCONNECT:
        break;
    case PatchCanvas::ACTION_GROUP_DISCONNECT_ALL:
        break;
    case PatchCanvas::ACTION_GROUP_SPLIT:
        group_id = value1;
        PatchCanvas::splitGroup(group_id);
        break;
    case PatchCanvas::ACTION_GROUP_JOIN:
        group_id = value1;
        PatchCanvas::joinGroup(group_id);
        break;
    case PatchCanvas::ACTION_REQUEST_PORT_CONNECTION_LIST:
        break;
    case PatchCanvas::ACTION_REQUEST_GROUP_CONNECTION_LIST:
        break;
    default:
        break;
    }
}

CanvasTestApp::CanvasTestApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CanvasTestApp)
{
    ui->setupUi(this);

    scene = new PatchCanvas::PatchScene(this);
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

    PatchCanvas::set_options(&options);
    PatchCanvas::init(scene, canvas_callback, true);

    scene->rubberbandByTheme();

    // TEST
    PatchCanvas::addGroup(0, "Box with timer, splitted", true);
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
}

CanvasTestApp::~CanvasTestApp()
{
    PatchCanvas::clear();

    delete scene;
    delete ui;
}
