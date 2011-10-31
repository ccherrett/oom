#include "canvasline.h"
#include "canvasport.h"
#include "canvasportglow.h"

#include <QPainter>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;
extern options_t options;

CanvasLine::CanvasLine(CanvasPort* item1_, CanvasPort* item2_, QGraphicsItem* parent) :
    QGraphicsLineItem(parent, canvas.scene)
{
    if (options.fancy_eyecandy)
        setOpacity(0);

    item1 = item1_;
    item2 = item2_;
    port_type1 = item1->getPortType();
    port_type2 = item2->getPortType();

    glow = 0;
    locked = false;

    updateLinePos();
}

CanvasLine::~CanvasLine()
{
    if (glow)
        delete glow;
}

bool CanvasLine::isLocked()
{
    return locked;
}

void CanvasLine::setLocked(bool yesno)
{
    locked = yesno;
}

void CanvasLine::enableGlow(bool yesno)
{
    if (locked) return;
    if (options.fancy_eyecandy)
    {
        if (yesno)
        {
            glow = new CanvasPortGlow(port_type1, toGraphicsObject());
            setGraphicsEffect(glow);
        }
        else
        {
            setGraphicsEffect(0);
            if (glow)
                delete glow;
            glow = 0;
        }
    }

    updateLineGradient(yesno);
}

void CanvasLine::updateLinePos()
{
    if (item1->getPortMode() == PORT_MODE_OUTPUT)
    {
        QLineF line(item1->scenePos().x() + item1->getPortWidth()+12, item1->scenePos().y()+7.5, item2->scenePos().x(), item2->scenePos().y()+7.5);
        setLine(line);
        updateLineGradient();
    }
}

void CanvasLine::updateLineGradient(bool selected)
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
        qWarning("Error: Invalid Port1 Type!");
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
        qWarning("Error: Invalid Port2 Type!");
        return;
    }

    setPen(QPen(port_gradient, 2));
}

int CanvasLine::type() const
{
    return CanvasLineType;
}

void CanvasLine::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, bool(options.antialiasing));
    QGraphicsLineItem::paint(painter, option, widget);
}

END_NAMESPACE_PATCHCANVAS
