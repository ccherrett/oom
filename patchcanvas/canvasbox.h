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

    int getGroupId();
    QString getGroupName();
    bool isSplitted();
    PortMode getSplittedMode();

    int getPortCount();
    QList<int> getPortList();

    void setIcon(Icon icon);
    void setSplit(bool split, PortMode mode=PORT_MODE_NULL);
    void setGroupName(QString group_name);

    void makeItGlow(int port_id, bool yesno);

    void addLineFromGroup(QGraphicsItem* line, int connection_id);
    void removeLineFromGroup(int connection_id);

    CanvasPort* addPortFromGroup(int port_id, QString port_name, PortMode port_mode, PortType port_type);
    void removePortFromGroup(int port_id);

    void checkItemPos();
    void removeIconFromScene();

    void updatePositions();
    void resetLinesZValue();
    void repaintLines();

    int type() const;

private:
    int group_id;
    QString group_name;

    int box_width;
    int box_height;

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
