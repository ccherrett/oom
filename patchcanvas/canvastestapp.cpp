#include "canvastestapp.h"
#include "ui_canvastestapp.h"

#include <cstdio>

static int last_connection_id = 0;

static void canvas_callback(PatchCanvas::CallbackAction action, int value1, int value2, QString value_str)
{
    printf("--------------------------- Callback called %i|%i|%i|%s\n", action, value1, value2, value_str.toStdString().data());

    int group_id;

    switch (action)
    {
    case PatchCanvas::ACTION_PORTS_CONNECT:
        PatchCanvas::connectPorts(last_connection_id++, value1, value2);
        break;
    case PatchCanvas::ACTION_PORTS_DISCONNECT:
        PatchCanvas::disconnectPorts(value1);
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
    options.bezier_lines = true;
    options.fancy_eyecandy = false;
    options.theme_name = Theme::getThemeName(Theme::getDefaultTheme());

    PatchCanvas::set_options(&options);
    PatchCanvas::init(scene, canvas_callback, true);

    scene->rubberbandByTheme();

    // TEST
    PatchCanvas::addGroup(0, "Box with timer, splitted", PatchCanvas::SPLIT_YES);
    PatchCanvas::addPort(0, 0, "AudioJackInputPort", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_AUDIO_JACK);
    PatchCanvas::addPort(0, 1, "AudioJackOutputPort", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_AUDIO_JACK);

    PatchCanvas::addPort(0, 2, "MidiJackInputPort", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_MIDI_JACK);
    PatchCanvas::addPort(0, 3, "MidiJackOutputPort", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_MIDI_JACK);

    PatchCanvas::addPort(0, 4, "MidiA2JInputPort", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_MIDI_A2J);
    PatchCanvas::addPort(0, 5, "MidiA2JOutputPort", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_MIDI_A2J);

    PatchCanvas::addPort(0, 6, "MidiAlsaInputPort", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_MIDI_ALSA);
    PatchCanvas::addPort(0, 7, "MidiAlsaOutputPort", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_MIDI_ALSA);

    PatchCanvas::addGroup(1, "Simple box", PatchCanvas::SPLIT_NO);
    PatchCanvas::addPort(1, 8, "Some Random Port 1", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_MIDI_JACK);
    PatchCanvas::addPort(1, 9, "Some Random Port 2", PatchCanvas::PORT_MODE_OUTPUT, PatchCanvas::PORT_TYPE_MIDI_ALSA);
    PatchCanvas::addPort(1, 10, "An input", PatchCanvas::PORT_MODE_INPUT, PatchCanvas::PORT_TYPE_AUDIO_JACK);

    PatchCanvas::connectPorts(11, 1, 10);
}

CanvasTestApp::~CanvasTestApp()
{
    PatchCanvas::clear();

    delete scene;
    delete ui;
}
