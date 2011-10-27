#ifndef CANVASPORT_H
#define CANVASPORT_H

#include "patchcanvas.h"
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

START_NAMESPACE_PATCHCANVAS

class CanvasPort : public QGraphicsItem
{
public:
    CanvasPort(int port_id, QString port_name, PortMode port_mode, PortType port_type, QGraphicsItem* parent);

    int getPortId();
    PortMode getPortMode();
    PortType getPortType();
    QString getPortName();
    int getPortWidth();
    int getPortHeight();

    void setPortMode(PortMode port_mode);
    void setPortType(PortType port_type);
    void setPortName(QString port_name);
    void setPortWidth(int port_width);
    void setPortHeight(int port_height);

    int type() const;

private:
    int port_id;
    PortMode port_mode;
    PortType port_type;
    QString port_name;

    int port_width;
    int port_height;
    QFont port_font;

    QGraphicsItem* mov_line;
    CanvasPort* hover_item;
    bool last_selected_state;

    bool mouse_down;
    bool moving_cursor;

    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASPORT_H
