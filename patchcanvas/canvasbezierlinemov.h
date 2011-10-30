#ifndef CANVASBEZIERLINEMOV_H
#define CANVASBEZIERLINEMOV_H

#include "patchcanvas.h"
#include <QGraphicsPathItem>
#include <QPainter>

START_NAMESPACE_PATCHCANVAS

class CanvasBezierLineMov : public QGraphicsPathItem
{
public:
    CanvasBezierLineMov(PortMode port_mode, PortType port_type, QGraphicsItem* parent);

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

#endif // CANVASBEZIERLINEMOV_H
