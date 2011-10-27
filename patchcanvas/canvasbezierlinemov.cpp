#include "canvasbezierlinemov.h"
#include "canvasport.h"

#include <cstdio>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;
extern options_t options;

CanvasBezierLineMov::CanvasBezierLineMov(QGraphicsItem* parent) :
    QGraphicsPathItem(parent, canvas.scene)
{
    port_mode = PORT_MODE_NULL;
    port_type = PORT_TYPE_NULL;

    // Port position doesn't change while moving around line
    item_x = parentItem()->scenePos().x();
    item_y = parentItem()->scenePos().y();
    item_width = ((CanvasPort*)parentItem())->getPortWidth(); // FIXME
}

void CanvasBezierLineMov::setPortMode(PortMode port_mode_)
{
    port_mode = port_mode_;
    update();
}

void CanvasBezierLineMov::setPortType(PortType port_type_)
{
    port_type = port_type_;

    QPen pen;

    if (port_type == PORT_TYPE_AUDIO)
      pen = QPen(canvas.theme->line_audio, 2);
    else if (port_type == PORT_TYPE_MIDI)
      pen = QPen(canvas.theme->line_midi, 2);
    else if (port_type == PORT_TYPE_OUTRO)
      pen = QPen(canvas.theme->line_outro, 2);
    else
    {
      printf("Error: Invalid Port Type!\n");
      return;
    }

    QColor color(0,0,0);
    color.setAlpha(0);
    setBrush(color);
    setPen(pen);
    update();
}

void CanvasBezierLineMov::updateLinePos(QPointF scenePos)
{
    int old_x, old_y, mid_x, new_x, final_x, final_y;

    if (port_mode == PORT_MODE_INPUT)
    {
      old_x = 0;
      old_y = 7.5;
      mid_x = abs(scenePos.x()-item_x)/2;
      new_x = old_x-mid_x;
    }
    else if (port_mode == PORT_MODE_OUTPUT)
    {
      old_x = item_width+12;
      old_y = 7.5;
      mid_x = abs(scenePos.x()-(item_x+old_x))/2;
      new_x = old_x+mid_x;
    }
    else
      return;

    // mid_fx = abs(old_x-scenePos.x())/2;
    // new_fx = scenePos.x()-mid_x;
    final_x = scenePos.x()-item_x;
    final_y = scenePos.y()-item_y;

    QPainterPath path(QPointF(old_x, old_y));
    path.cubicTo(new_x, old_y, new_x, final_y, final_x, final_y);
    setPath(path);
    update();
}

int CanvasBezierLineMov::type() const
{
    return CanvasBezierLineMovType;
}

void CanvasBezierLineMov::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, bool(options.antialiasing));
    QGraphicsPathItem::paint(painter, option, widget);
}

END_NAMESPACE_PATCHCANVAS
