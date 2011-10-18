#ifndef PATCHCANVAS_H
#define PATCHCANVAS_H

#include <QSettings>
#include <QTimer>

#include "patchscene.h"

namespace PatchCanvas {
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

// Main Canvas object
class Canvas {
public:
    Canvas();
    ~Canvas();
    void init(PatchScene* scene, void* callback, bool debug=false);

    options_t options;
    features_t features;

    PatchScene* scene;
    void* callback;
    bool debug;
    int last_z_value;
    int last_group_id;
    int last_connection_id;
    Theme* theme;

private:
//    QPointF initial_pos;
//    QList<> group_list;
//    QList<> port_list;
//    QList<> connection_list;
//    QList<> postponed_groups;
    QTimer postponed_timer;
    QSettings* settings;
//    QRectF size_rect;
};

}

#endif // PATCHCANVAS_H
