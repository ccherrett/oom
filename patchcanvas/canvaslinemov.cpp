#include "canvaslinemov.h"
#include "canvasport.h"

#include <QPainter>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;
extern options_t options;

CanvasLineMov::CanvasLineMov(PortMode port_mode, PortType port_type, QGraphicsItem* parent) :
    QGraphicsLineItem(parent, canvas.scene)
{
    port_mode = port_mode;
    port_type = port_type;

    // Port position doesn't change while moving around line
    item_x = parentItem()->scenePos().x();
    item_y = parentItem()->scenePos().y();
    item_width = ((CanvasPort*)parentItem())->getPortWidth();

    QPen pen;

    if (port_type == PORT_TYPE_AUDIO)
      pen = QPen(canvas.theme->line_audio, 2);
    else if (port_type == PORT_TYPE_MIDI)
      pen = QPen(canvas.theme->line_midi, 2);
    else if (port_type == PORT_TYPE_OUTRO)
      pen = QPen(canvas.theme->line_outro, 2);
    else
    {
      qWarning("Error: Invalid Port Type!");
      return;
    }

    setPen(pen);
    update();
}

void CanvasLineMov::updateLinePos(QPointF scenePos)
{
    int item_pos[2];

    if (port_mode == PORT_MODE_INPUT)
    {
        item_pos[0] = 0;
        item_pos[1] = 7.5;
    }
    else if (port_mode == PORT_MODE_OUTPUT)
    {
        item_pos[0] = item_width+12;
        item_pos[1] = 7.5;
    }
    else
      return;

    QLineF line(item_pos[0], item_pos[1], scenePos.x()-line_x, scenePos.y()-line_y);
    setLine(line);
    update();
}

int CanvasLineMov::type() const
{
    return CanvasLineMovType;
}

void CanvasLineMov::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, bool(options.antialiasing));
    QGraphicsLineItem::paint(painter, option, widget);
}

END_NAMESPACE_PATCHCANVAS
