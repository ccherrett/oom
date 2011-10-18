#ifndef CANVASBOX_H
#define CANVASBOX_H

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QFont>
#include <QPainter>

#include "patchcanvas.h"
#include "canvasicon.h"

START_NAMESPACE_PATCHCANVAS

class CanvasBoxShadow;

class CanvasBox : public QGraphicsItem
{
public:
    CanvasBox(int group_id, QString text, Icon icon, QGraphicsScene* scene);
    ~CanvasBox();

    void setIcon(Icon icon);
    void setSplit(bool split, PortMode mode=PORT_MODE_NULL);
    void setText(QString text);

    //makeItGlow(self, port_id, glow)
    //addLine(self, line, connection_id)
    //removeLine(self, connection_id)
    //addPort(self, port_id, port_name, port_mode, port_type)
    //removePort(self, port_id)
    //renamePort(self, port_id, new_port_name)

    void relocateAll();
    void resetLinesZValue();
    void repaintLines();

    CanvasIcon* icon_svg;

private:
    int group_id;
    QString text;

    int box_width;
    int box_height;

    bool splitted;
    PortMode splitted_mode;
    bool forced_split;
    bool moving_cursor;
    bool mouse_down;

    QFont font_name;
    QFont font_port;

    CanvasBoxShadow* shadow;

    //checkItemPos(self)
    //contextMenuEvent(self, event)
    //contextMenuDisconnect(self, port_id)

    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);    
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASBOX_H
