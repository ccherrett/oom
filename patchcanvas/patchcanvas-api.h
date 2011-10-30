#ifndef PATCHCANVASAPI_H
#define PATCHCANVASAPI_H

#include <Qt>
#include <QString>

class QGraphicsScene;

#define START_NAMESPACE_PATCHCANVAS namespace PatchCanvas {
#define END_NAMESPACE_PATCHCANVAS }

START_NAMESPACE_PATCHCANVAS

enum PortMode {
    PORT_MODE_NULL   = 0,
    PORT_MODE_INPUT  = 1,
    PORT_MODE_OUTPUT = 2
};

enum PortType {
    PORT_TYPE_NULL  = 0,
    PORT_TYPE_AUDIO = 1,
    PORT_TYPE_MIDI  = 2,
    PORT_TYPE_OUTRO = 3
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

// API starts here
void set_options(options_t* options);
void set_features(features_t* features);
void init(QGraphicsScene* scene, void* callback, bool debug=false);
void clear();

void setInitialPos(int x, int y);
void setCanvasSize(int x, int y, int width, int height);

void addGroup(int group_id, QString group_name, bool split=false, Icon icon=ICON_APPLICATION);
void removeGroup(int group_id);
void renameGroup(int group_id, QString new_name);
void splitGroup(int group_id);
void joinGroup(int group_id);
void setGroupPos(int group_id, int group_pos_x, int group_pos_y, int group_pos_xs, int group_pos_ys);
void setGroupIcon(int group_id, Icon icon);

void addPort(int group_id, int port_id, QString port_name, PortMode port_mode, PortType port_type);
void removePort(int port_id);
void renamePort(int port_id, QString new_port_name);

void connectPorts(int connection_id, int port_out_id, int port_in_id);
void disconnectPorts(int connection_id);

void Arrange();

END_NAMESPACE_PATCHCANVAS

#endif // PATCHCANVASAPI_H
