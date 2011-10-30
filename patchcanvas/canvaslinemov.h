#ifndef CANVASLINEMOV_H
#define CANVASLINEMOV_H

#include "patchcanvas.h"

#include <QGraphicsLineItem>

class QPainter;

START_NAMESPACE_PATCHCANVAS

class CanvasLineMov : public QGraphicsLineItem
{
public:
    CanvasLineMov(PortMode port_mode, PortType port_type, QGraphicsItem* parent);

    void updateLinePos(QPointF scenePos);

    int type() const;

private:
    PortMode port_mode;
    PortType port_type;
    int item_x;
    int item_y;
    int item_width;
    int line_x;
    int line_y;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASLINEMOV_H
