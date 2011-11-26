#ifndef PATCHCANVAS_H
#define PATCHCANVAS_H

#include "theme.h"
#include "patchcanvas-api.h"

#include <QGraphicsItem>
#include <QGraphicsScene>

class QSettings;
class QTimer;

class CanvasObject : public QObject {
    Q_OBJECT

public:
    CanvasObject(QObject* parent=0);

public slots:
    void CanvasPostponedGroups();
    void PortContextMenuDisconnect();
};

START_NAMESPACE_PATCHCANVAS

// Forward declarations
class CanvasBox;
class CanvasPort;
class CanvasFadeAnimation;

// object types
enum CanvasType {
    CanvasBoxType           = QGraphicsItem::UserType + 1,
    CanvasIconType          = QGraphicsItem::UserType + 2,
    CanvasPortType          = QGraphicsItem::UserType + 3,
    CanvasLineType          = QGraphicsItem::UserType + 4,
    CanvasBezierLineType    = QGraphicsItem::UserType + 5,
    CanvasLineMovType       = QGraphicsItem::UserType + 6,
    CanvasBezierLineMovType = QGraphicsItem::UserType + 7
};

// object lists
struct group_dict_t {
    int group_id;
    QString group_name;
    bool split;
    Icon icon;
    CanvasBox* widgets[2];
};

struct port_dict_t {
    int group_id;
    int port_id;
    QString port_name;
    PortMode port_mode;
    PortType port_type;
    CanvasPort* widget;
};

struct connection_dict_t {
    int connection_id;
    int port_in_id;
    int port_out_id;
    QGraphicsItem* widget;
};

struct animation_dict_t {
    CanvasFadeAnimation* animation;
    QGraphicsItem* item;
};

// Main Canvas object
class Canvas {
public:
    Canvas();
    ~Canvas();

    QGraphicsScene* scene;
    Callback callback;
    bool debug;
    unsigned long last_z_value;
    int last_group_id;
    int last_connection_id;
    QPointF initial_pos;
    QList<group_dict_t> group_list;
    QList<port_dict_t> port_list;
    QList<connection_dict_t> connection_list;
    QList<animation_dict_t> animation_list;
    QList<int> postponed_groups;
    CanvasObject* qobject;
    QSettings* settings;
    Theme* theme;
    QRectF size_rect;
    bool initiated;
};

QString CanvasGetGroupName(int group_id);
int CanvasGetGroupPortCount(int group_id);
QPointF CanvasGetNewGroupPos(bool horizontal=false);
//QList<int> CanvasGetGroupConnectionList(int group_id);

QString CanvasGetPortName(int port_id);
QList<int> CanvasGetPortConnectionList(int port_id);

int CanvasGetConnectedPort(int connection_id, int port_id);

void CanvasPostponedGroups();
void CanvasCallback(CallbackAction action, int value1, int value2, QString value_str);

void ItemFX(QGraphicsItem* item, bool show, bool destroy=true);
void RemoveItemFX(QGraphicsItem* item);

END_NAMESPACE_PATCHCANVAS

#endif // PATCHCANVAS_H
