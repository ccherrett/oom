#ifndef PATCHCANVAS_H
#define PATCHCANVAS_H

#include <QGraphicsScene>
#include <QSettings>
#include <QTimer>

#include "theme.h"

#define START_NAMESPACE_PATCHCANVAS namespace PatchCanvas {
#define END_NAMESPACE_PATCHCANVAS }

START_NAMESPACE_PATCHCANVAS

enum PortMode {
    PORT_MODE_NULL   = 0,
    PORT_MODE_INPUT  = 1,
    PORT_MODE_OUTPUT = 2
};

enum PortType {
    PORT_TYPE_AUDIO = 0,
    PORT_TYPE_MIDI  = 1,
    PORT_TYPE_OUTRO = 2
};

enum CallbackAction {
    ACTION_PORT_DISCONNECT_ALL  = 0,
    ACTION_PORT_RENAME          = 1,
    ACTION_PORT_INFO            = 2,
    ACTION_PORTS_CONNECT        = 3,
    ACTION_PORTS_DISCONNECT     = 4,
    ACTION_GROUP_DISCONNECT_ALL = 5,
    ACTION_GROUP_RENAME         = 6,
    ACTION_GROUP_INFO           = 7,
    ACTION_GROUP_SPLIT          = 8,
    ACTION_GROUP_JOIN           = 9,
    ACTION_REQUEST_PORT_CONNECTION_LIST  = 10,
    ACTION_REQUEST_GROUP_CONNECTION_LIST = 11
};

enum Icon {
    ICON_HARDWARE    = 0,
    ICON_APPLICATION = 1,
    ICON_LADISH_ROOM = 2
};

// Forward declarations
class CanvasBox;

// Canvas options
struct options_t {
    QString theme_name;
    bool bezier_lines;
    Qt::CheckState antialiasing;
    bool auto_hide_groups;
    bool connect_midi2outro;
    bool fancy_eyecandy;
};

// Canvas features
struct features_t {
    bool group_rename;
    bool port_rename;
    bool handle_group_pos;
};

struct group_dict_t {
    int id;
    QString name;
    bool split;
    Icon icon;
    CanvasBox* widgets[2];
};

// Main Canvas object
class Canvas {
public:
    Canvas();
    ~Canvas();

    QGraphicsScene* scene;
    void* callback;
    bool debug;
    int last_z_value;
    int last_group_id;
    int last_connection_id;
    QPointF initial_pos;
    QList<group_dict_t> group_list;
    //    QList<> port_list;
    //    QList<> connection_list;
    QList<int> postponed_groups;
    QTimer postponed_timer;
    QSettings* settings;
    Theme* theme;
    QRectF size_rect;
};

// API starts here
void init(QGraphicsScene* scene, void* callback, bool debug=false);
void clear();

void setInitialPos(int x, int y);
void setCanvasSize(int x, int y, int width, int height);

void addGroup(int group_id, const char* group_name, bool split=false, Icon icon=ICON_APPLICATION);
void removeGroup(int group_id);
void renameGroup(int group_id, const char* new_name);

END_NAMESPACE_PATCHCANVAS

#endif // PATCHCANVAS_H
