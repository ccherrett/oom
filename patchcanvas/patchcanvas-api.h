#ifndef PATCHCANVASAPI_H
#define PATCHCANVASAPI_H

#include <Qt>
#include <QString>

#define START_NAMESPACE_PATCHCANVAS namespace PatchCanvas {
#define END_NAMESPACE_PATCHCANVAS }

START_NAMESPACE_PATCHCANVAS

class PatchScene;

enum PortMode {
    PORT_MODE_NULL   = 0,
    PORT_MODE_INPUT  = 1,
    PORT_MODE_OUTPUT = 2
};

enum PortType {
    PORT_TYPE_NULL       = 0,
    PORT_TYPE_AUDIO_JACK = 1,
    PORT_TYPE_MIDI_JACK  = 2,
    PORT_TYPE_MIDI_A2J   = 3,
    PORT_TYPE_MIDI_ALSA  = 4
};

enum CallbackAction {
    ACTION_GROUP_INFO       = 0, // group_id, N, N
    ACTION_GROUP_RENAME     = 1, // group_id, N, new_name
    ACTION_GROUP_SPLIT      = 2, // group_id, N, N
    ACTION_GROUP_JOIN       = 3, // group_id, N, N
    ACTION_PORT_INFO        = 4, // port_id, N, N
    ACTION_PORT_RENAME      = 5, // port_id, N, new_name
    ACTION_PORTS_CONNECT    = 6, // out_id, in_id, N
    ACTION_PORTS_DISCONNECT = 7  // conn_id, N, N
};

enum Icon {
    ICON_HARDWARE    = 0,
    ICON_APPLICATION = 1,
    ICON_LADISH_ROOM = 2
};

enum SplitOption {
    SPLIT_UNDEF = 0,
    SPLIT_NO    = 1,
    SPLIT_YES   = 2
};

// Canvas options
struct options_t {
    QString theme_name;
    bool bezier_lines;
    Qt::CheckState antialiasing;
    bool auto_hide_groups;
    bool fancy_eyecandy;
};

// Canvas features
struct features_t {
    bool group_info;
    bool group_rename;
    bool port_info;
    bool port_rename;
    bool handle_group_pos;
};

typedef void (*Callback) (CallbackAction action, int value1, int value2, QString value_str);

// API starts here
void set_options(options_t* options);
void set_features(features_t* features);
void init(PatchScene* scene, Callback callback, bool debug=false);
void clear();

void setInitialPos(int x, int y);
void setCanvasSize(int x, int y, int width, int height);

void addGroup(int group_id, QString group_name, SplitOption split=SPLIT_UNDEF, Icon icon=ICON_APPLICATION);
void removeGroup(int group_id);
void renameGroup(int group_id, QString new_group_name);
void splitGroup(int group_id);
void joinGroup(int group_id);
void setGroupPos(int group_id, int group_pos_x, int group_pos_y);
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
