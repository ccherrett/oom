#ifndef CANVASPORT_H
#define CANVASPORT_H

#include "patchcanvas.h"

#include <QGraphicsItem>

class QGraphicsSceneContextMenuEvent;
class QGraphicsSceneMouseEvent;
class QPainter;

START_NAMESPACE_PATCHCANVAS

class CanvasPort : public QGraphicsItem
{
public:
    CanvasPort(int port_id, QString port_name, PortMode port_mode, PortType port_type, QGraphicsItem* parent);
    CanvasPort() {}
    ~CanvasPort() {}

    int getPortId();
    PortMode getPortMode();
    PortType getPortType();
    QString getPortName();
    QString getFullPortName();
    int getPortWidth();
    int getPortHeight();

    void setPortMode(PortMode port_mode);
    void setPortType(PortType port_type);
    void setPortName(QString port_name);
    void setPortWidth(int port_width);

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
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASPORT_H
