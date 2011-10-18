#ifndef CANVASBOX_H
#define CANVASBOX_H

#include <QGraphicsItem>
#include <QFont>
#include <QPainter>

#include "patchcanvas.h"
#include "canvasicon.h"

class CanvasBox : public QGraphicsItem
{
public:
    CanvasBox(int group_id, QString text, PatchCanvas::Icon icon, PatchScene* scene, PatchCanvas::Canvas* canvas);
    ~CanvasBox();

    void setIcon(PatchCanvas::Icon icon);
    void setSplit(bool split, PatchCanvas::PortMode mode);
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

private:
    PatchCanvas::Canvas* canvas;

    int group_id;
    QString text;

    int box_width;
    int box_height;

    bool splitted;
    PatchCanvas::PortMode splitted_mode;
    bool forced_split;
    bool moving_cursor;
    bool mouse_down;

    QFont font_name;
    QFont font_port;

    CanvasIcon* icon_svg;

    //checkItemPos(self)
    //contextMenuEvent(self, event)
    //contextMenuDisconnect(self, port_id)

    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);    
};

#endif // CANVASBOX_H
