#include "patchcanvas.h"

#include <cstdio>

#ifndef PATCHCANVAS_ORGANISATION_NAME
#define PATCHCANVAS_ORGANISATION_NAME "OpenOctave"
#endif

namespace PatchCanvas {

const char* bool2str(bool check)
{
    return check ? "True" : "False";
}

Canvas::Canvas()
{
    //options.theme_name = getDefaultThemeName();
    options.bezier_lines = true;
    options.antialiasing = Qt::PartiallyChecked;
    options.auto_hide_groups = true;
    options.connect_midi2outro = false;
    options.fancy_eyecandy = false;

    features.group_rename = true;
    features.port_rename = true;
    features.handle_group_pos = false;

    settings = 0;
}

Canvas::~Canvas()
{
    if (settings)
        delete settings;
}

void Canvas::init(PatchScene* scene_, void* callback_, bool debug_)
{
    if (debug)
        printf("patchcanvas::init(%p, %p, %s)\n", scene, callback, bool2str(debug));

    scene = scene_;
    callback = callback_;
    debug = debug_;

    last_z_value = 0;
    last_group_id = 0;
    last_connection_id = 0;
    //initial_pos = QPointF(0, 0);

    postponed_timer.setInterval(100);
    //QObject::connect(postponed_timer, SIGNAL("timeout()"), CanvasPostponedGroups);

    settings = new QSettings(PATCHCANVAS_ORGANISATION_NAME, "PatchCanvas");

    //for i in range(len(theme_list))
    //  if (theme_list[i]['name'] == options['theme_name'])
    //  {
    //    Canvas.theme = theme_list[i]
    //    break;
    //  }
    //else
    //  Canvas.theme = getDefaultTheme()
    theme = new Theme();

    //size_rect = QRectF();

    scene->setBackgroundBrush(theme->canvas_bg);
}

}
