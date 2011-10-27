#ifndef CANVASLINEMOV_H
#define CANVASLINEMOV_H

#include "patchcanvas.h"
#include <QGraphicsLineItem>
#include <QPainter>

START_NAMESPACE_PATCHCANVAS

class CanvasLineMov : public QGraphicsLineItem
{
public:
    CanvasLineMov(QGraphicsItem* parent);

    void setPortMode(PortMode port_mode);
    void setPortType(PortType port_type);
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
