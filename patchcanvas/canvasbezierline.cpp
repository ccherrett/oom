#include "canvasbezierline.h"
#include "canvasport.h"
#include "canvasportglow.h"

#include <QPainter>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;
extern options_t options;

CanvasBezierLine::CanvasBezierLine(CanvasPort* item1_, CanvasPort* item2_, QGraphicsItem* parent) :
    QGraphicsPathItem(parent, canvas.scene)
{
    //if (options.fancy_eyecandy)
        //setOpacity(0);

    item1 = item1_;
    item2 = item2_;

    locked = false;
    line_selected = false;

    setBrush(QColor(0,0,0,0));
    setGraphicsEffect(0);
    updateLinePos();
}

CanvasBezierLine::~CanvasBezierLine()
{
    setGraphicsEffect(0);
}

bool CanvasBezierLine::isLocked()
{
    return locked;
}

void CanvasBezierLine::setLocked(bool yesno)
{
    locked = yesno;
}

bool CanvasBezierLine::isLineSelected()
{
    return line_selected;
}

void CanvasBezierLine::setLineSelected(bool yesno)
{
    if (locked) return;
    if (options.fancy_eyecandy)
    {
        if (yesno)
            setGraphicsEffect(new CanvasPortGlow(item1->getPortType(), toGraphicsObject()));
        else
            setGraphicsEffect(0);
    }

    updateLineGradient(yesno);
    line_selected = true;
}

void CanvasBezierLine::updateLinePos()
{
    if (item1->getPortMode() == PORT_MODE_OUTPUT)
    {
        int item1_x = item1->scenePos().x() + item1->getPortWidth()+12;
        int item1_y = item1->scenePos().y() + 7.5;

        int item2_x = item2->scenePos().x();
        int item2_y = item2->scenePos().y()+7.5;

        int item1_mid_x = abs(item1_x-item2_x)/2;
        int item1_new_x = item1_x+item1_mid_x;

        int item2_mid_x = abs(item1_x-item2_x)/2;
        int item2_new_x = item2_x-item2_mid_x;

        QPainterPath path(QPointF(item1_x, item1_y));
        path.cubicTo(item1_new_x, item1_y, item2_new_x, item2_y, item2_x, item2_y);
        setPath(path);

        line_selected = false;
        updateLineGradient(line_selected);
    }
}

void CanvasBezierLine::updateLineGradient(bool selected)
{
    int pos_top = boundingRect().top();
    int pos_bot = boundingRect().bottom();
    short pos1, pos2;

    if (item2->scenePos().y() >= item1->scenePos().y())
    {
        pos1 = 0;
        pos2 = 1;
    }
    else
    {
        pos1 = 1;
        pos2 = 0;
    }

    PortType port_type1 = item1->getPortType();
    PortType port_type2 = item2->getPortType();
    QLinearGradient port_gradient(0, pos_top, 0, pos_bot);

    if (port_type1 == PORT_TYPE_AUDIO_JACK)
        port_gradient.setColorAt(pos1, selected ? canvas.theme->line_audio_jack_sel : canvas.theme->line_audio_jack);
    else if (port_type1 == PORT_TYPE_MIDI_JACK)
        port_gradient.setColorAt(pos1, selected ? canvas.theme->line_midi_jack_sel : canvas.theme->line_midi_jack);
    else if (port_type1 == PORT_TYPE_MIDI_A2J)
        port_gradient.setColorAt(pos1, selected ? canvas.theme->line_midi_a2j_sel : canvas.theme->line_midi_a2j);
    else if (port_type1 == PORT_TYPE_MIDI_ALSA)
        port_gradient.setColorAt(pos1, selected ? canvas.theme->line_midi_alsa_sel : canvas.theme->line_midi_alsa);

    if (port_type2 == PORT_TYPE_AUDIO_JACK)
        port_gradient.setColorAt(pos2, selected ? canvas.theme->line_audio_jack_sel : canvas.theme->line_audio_jack);
    else if (port_type2 == PORT_TYPE_MIDI_JACK)
        port_gradient.setColorAt(pos2, selected ? canvas.theme->line_midi_jack_sel : canvas.theme->line_midi_jack);
    else if (port_type2 == PORT_TYPE_MIDI_A2J)
        port_gradient.setColorAt(pos2, selected ? canvas.theme->line_midi_a2j_sel : canvas.theme->line_midi_a2j);
    else if (port_type2 == PORT_TYPE_MIDI_ALSA)
        port_gradient.setColorAt(pos2, selected ? canvas.theme->line_midi_alsa_sel : canvas.theme->line_midi_alsa);

    setPen(QPen(port_gradient, 2));
}

int CanvasBezierLine::type() const
{
    return CanvasBezierLineType;
}

void CanvasBezierLine::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, bool(options.antialiasing));
    QGraphicsPathItem::paint(painter, option, widget);
}

END_NAMESPACE_PATCHCANVAS
