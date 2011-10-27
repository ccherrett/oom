#include "canvasbezierline.h"
#include "canvasport.h"
#include "canvasportglow.h"

#include <cstdio>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;
extern options_t options;

CanvasBezierLine::CanvasBezierLine(CanvasPort* item1_, CanvasPort* item2_, QGraphicsItem* parent) :
    QGraphicsPathItem(parent, canvas.scene)
{
    if (options.fancy_eyecandy)
        setOpacity(0);

    item1 = item1_;
    item2 = item2_;
    port_type1 = item1->getPortType();
    port_type2 = item2->getPortType();

    glow = 0;
    locked = false;

    setBrush(QColor(0,0,0,0));
    updateLinePos();
}

CanvasBezierLine::~CanvasBezierLine()
{
    if (glow)
        delete glow;
}

void CanvasBezierLine::enableGlow(bool yesno)
{
    if (locked) return;
    if (options.fancy_eyecandy)
    {
        if (yesno)
        {
            glow = new CanvasPortGlow(toGraphicsObject());
            glow->setPortType(port_type1);
        }
        else
        {
            if (glow)
                delete glow;
            glow = 0;
        }
    }

    updateLineGradient(yesno);
}

void CanvasBezierLine::setPortType(PortType port_type1_, PortType port_type2_)
{
    port_type1 = port_type1_;
    port_type2 = port_type2_;
    updateLineGradient();
}

void CanvasBezierLine::setLocked(bool yesno)
{
    locked = yesno;
}

bool CanvasBezierLine::isLocked()
{
    return locked;
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
        updateLineGradient();
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

    QLinearGradient port_gradient(0, pos_top, 0, pos_bot);

    if (port_type1 == PORT_TYPE_AUDIO)
        port_gradient.setColorAt(pos1, selected ? canvas.theme->line_audio_sel : canvas.theme->line_audio);
    else if (port_type1 == PORT_TYPE_MIDI)
        port_gradient.setColorAt(pos1, selected ? canvas.theme->line_midi_sel : canvas.theme->line_midi);
    else if (port_type1 == PORT_TYPE_OUTRO)
        port_gradient.setColorAt(pos1, selected ? canvas.theme->line_outro_sel : canvas.theme->line_outro);
    else
    {
        printf("Error: Invalid Port1 Type!\n");
        return;
    }

    if (port_type2 == PORT_TYPE_AUDIO)
        port_gradient.setColorAt(pos2, selected ? canvas.theme->line_audio_sel : canvas.theme->line_audio);
    else if (port_type2 == PORT_TYPE_MIDI)
        port_gradient.setColorAt(pos2, selected ? canvas.theme->line_midi_sel : canvas.theme->line_midi);
    else if (port_type2 == PORT_TYPE_OUTRO)
        port_gradient.setColorAt(pos2, selected ? canvas.theme->line_outro_sel : canvas.theme->line_outro);
    else
    {
        printf("Error: Invalid Port2 Type!\n");
        return;
    }

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
