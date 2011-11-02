#ifndef CANVASBOX_H
#define CANVASBOX_H

#include "patchcanvas.h"
#include "canvasicon.h"
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QFont>
#include <QPainter>

START_NAMESPACE_PATCHCANVAS

class CanvasBoxShadow;
class CanvasPort;

struct cb_line_t {
    QGraphicsItem* line;
    int connection_id;
};

class CanvasBox : public QGraphicsItem
{
public:
    CanvasBox(int group_id, QString text, Icon icon, QGraphicsItem* parent=0);
    ~CanvasBox();

    QString getText();
    bool isSplitted();
    PortMode getSplittedMode();

    QList<port_dict_t> getPortList();
    int getPortCount();

    void setIcon(Icon icon);
    void setSplit(bool split, PortMode mode=PORT_MODE_NULL);
    void setText(QString text);

    void makeItGlow(int port_id, bool yesno);

    void addLine(QGraphicsItem* line, int connection_id);
    void removeLine(int connection_id);

    CanvasPort* addPort(int port_id, QString port_name, PortMode port_mode, PortType port_type);
    void removePort(int port_id);
    void renamePort(int port_id, QString new_port_name);

    void removeIconFromScene();

    void relocateAll();
    void resetLinesZValue();
    void repaintLines();

    int type() const;

private:
    int group_id;
    QString text;

    int box_width;
    int box_height;

    QList<port_dict_t> port_list;
    QList<int> port_list_ids;
    QList<cb_line_t> connection_lines;

    QPointF last_pos;
    bool splitted;
    PortMode splitted_mode;

    bool forced_split;
    bool moving_cursor;
    bool mouse_down;

    QFont font_name;
    QFont font_port;

    CanvasIcon* icon_svg;
    CanvasBoxShadow* shadow;

    void checkItemPos();

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
