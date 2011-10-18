#include "patchcanvas.h"

#include <cstdio>

#ifndef PATCHCANVAS_ORGANISATION_NAME
#define PATCHCANVAS_ORGANISATION_NAME "PatchCanvas"
#endif

#include "canvasbox.h"

START_NAMESPACE_PATCHCANVAS

/* Global objects */
Canvas canvas;

options_t options = {
    /* theme_name */         QString(Theme::getThemeName(Theme::getDefaultTheme())),
    /* bezier_lines */       true,
    /* antialiasing */       Qt::PartiallyChecked,
    /* auto_hide_groups */   true,
    /* connect_midi2outro */ false,
    /* fancy_eyecandy */     true
};

features_t features = {
    /* group_rename */     true,
    /* port_rename */      true,
    /* handle_group_pos */ false

};

/* contructor and destructor */
Canvas::Canvas()
{
    settings = 0;
    theme = 0;
}

Canvas::~Canvas()
{
    if (settings)
        delete settings;
    if (theme)
        delete theme;
}

const char* bool2str(bool check)
{
    return check ? "true" : "false";
}

void init(QGraphicsScene* scene, void* callback, bool debug)
{
    if (debug)
        printf("patchcanvas::init(%p, %p, %s)\n", scene, callback, bool2str(debug));

    canvas.scene = scene;
    canvas.callback = callback;
    canvas.debug = debug;

    canvas.last_z_value = 0;
    canvas.last_group_id = 0;
    canvas.last_connection_id = 0;
    canvas.initial_pos = QPointF(0, 0);

    canvas.postponed_timer.setInterval(100);
    //QObject::connect(postponed_timer, SIGNAL("timeout()"), CanvasPostponedGroups);

    canvas.settings = new QSettings(PATCHCANVAS_ORGANISATION_NAME, "PatchCanvas");

    for (int i=0; i<Theme::THEME_MAX; i++)
    {
        QString this_theme_name = Theme::getThemeName((Theme::List)i);
        if (this_theme_name == options.theme_name)
        {
            canvas.theme = new Theme((Theme::List)i);
            break;
        }
    }

    if (!canvas.theme)
        canvas.theme = new Theme(Theme::getDefaultTheme());

    canvas.size_rect = QRectF();

    canvas.scene->setBackgroundBrush(canvas.theme->canvas_bg);
}

void clear()
{
    if (canvas.debug)
        printf("PatchCanvas::clear()\n");

    //  for i in range(len(Canvas.connection_list)):
    //    disconnectPorts(Canvas.connection_list[0][0])

    //  for i in range(len(Canvas.port_list)):
    //    removePort(Canvas.port_list[0]['id'])

    for (int i=0; i < canvas.group_list.count();) {
        removeGroup(canvas.group_list[0].id);
    }

    //  global animations
    //  for i in range(len(animations)):
    //    if (animations[0][0].state() == QAbstractAnimation.Running):
    //      animations[0][0].stop()
    //      RemoveItemFX(animations[0][1])
    //    animations.pop(0)

    canvas.last_z_value = 0;
    canvas.last_group_id = 0;
    canvas.last_connection_id = 0;

    canvas.group_list.clear();
    //  Canvas.port_list = [];
    //  Canvas.connection_list = [];
    //Canvas.postponed_groups.clear() // TESTING - force remove, or wait for removal?
    //Canvas.postponed_timer.stop()
}

void setInitialPos(int x, int y)
{
    if (canvas.debug)
        printf("PatchCanvas::setInitialPos(%i, %i)\n", x, y);

    canvas.initial_pos = QPointF(x, y);
}

void setCanvasSize(int x, int y, int width, int height)
{
    if (canvas.debug)
        printf("PatchcCnvas::setCanvasSize(%i, %i, %i, %i)\n", x, y, width, height);

    canvas.size_rect = QRectF(x, y, width, height);
}

void addGroup(int group_id, const char* group_name, bool split, Icon icon)
{
    if (canvas.debug)
        printf("PatchCanvas::addGroup(%i, %s, %s, %i)\n", group_id, group_name, bool2str(split), icon);

    CanvasBox* group_box = new CanvasBox(group_id, group_name, icon, canvas.scene);

    group_dict_t group_dict;
    group_dict.id = group_id;
    group_dict.name = QString(group_name);
    group_dict.split = split;
    group_dict.icon = icon;
    group_dict.widgets[0] = group_box;

    if (split)
    {
        group_box->setSplit(true, PORT_MODE_OUTPUT);

        //if (features.handle_group_pos)
        //group_box->setPos(canvas.settings.value(QString("CanvasPositions/%1_o".arg(group_name)), CanvasGetNewGroupPos()).toPointF());
        //else
        //group_box->setPos(CanvasGetNewGroupPos());

        CanvasBox* group_sbox = new CanvasBox(group_id, group_name, icon, canvas.scene);
        group_dict.widgets[1] = group_sbox;

        group_sbox->setSplit(true, PORT_MODE_INPUT);

        //    if (features['handle_group_pos']):
        //      group_sbox.setPos(Canvas.settings.value("CanvasPositions/%s_i" % (group_name), CanvasGetNewGroupPos(True)).toPointF())
        //    else:
        //      group_sbox.setPos(CanvasGetNewGroupPos(True))

        //if (not options['auto_hide_groups'] and options['fancy_eyecandy']):
        //ItemFX(group_sbox, True)

        canvas.last_z_value += 2;
        group_sbox->setZValue(canvas.last_z_value);
    }
    else
    {
        //bool horizontal;
        //if (strcmp(group_name, "Hardware Capture") == 0 || strcmp(group_name, "Hardware Playback") == 0 ||
        //        strcmp(group_name, "Capture") == 0 || strcmp(group_name, "Playback") == 0)
        //    horizontal = true;
        //else
        //    horizontal = false;

        group_box->setSplit(false);

        //    if (features['handle_group_pos']):
        //      group_box.setPos(Canvas.settings.value("CanvasPositions/%s" % (group_name), CanvasGetNewGroupPos()).toPointF())
        //    else:
        //      group_box.setPos(CanvasGetNewGroupPos(horizontal))
    }

    //if (not options['auto_hide_groups'] and options['fancy_eyecandy']):
    //ItemFX(group_box, True)

    canvas.last_z_value += 1;
    group_box->setZValue(canvas.last_z_value);

    canvas.group_list.append(group_dict);

    //QTimer::singleShot(0, canvas.scene->update);
}

void removeGroup(int group_id)
{
    if (canvas.debug)
        printf("PatchCanvas::removeGroup(%i)\n", group_id);

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].id == group_id)
        {
            CanvasBox* item = canvas.group_list[i].widgets[0];
            QString group_name = canvas.group_list[i].name;

            //if (item->port_list.count() > 0)
            //{
            //    if (canvas.debug)
            //        printf("PatchCanvas::removeGroup - This group still has ports, postpone it's removal\n");
            //    canvas.postponed_groups.append(group_id);
            //    //canvas.postponed_timer.start();
            //    return;
            //}

            if (canvas.group_list[i].split)
            {
                CanvasBox* s_item = canvas.group_list[i].widgets[1];
                if (features.handle_group_pos)
                {
                    canvas.settings->setValue(QString("CanvasPositions/%1_o").arg(group_name), item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_i").arg(group_name), s_item->pos());
                }

                //if (options.fancy_eyecandy && s_item->isVisible())
                //  ItemFX(s_item, False)
                //else
                canvas.scene->removeItem(s_item->icon_svg);
                canvas.scene->removeItem(s_item);

                delete item;
                delete s_item;
            }
            else
            {
                if (features.handle_group_pos)
                    canvas.settings->setValue(QString("CanvasPositions/%1").arg(group_name), item->pos());

                //if (options['fancy_eyecandy'] and item.isVisible())
                //  ItemFX(item, False)
                //else:
                canvas.scene->removeItem(item->icon_svg);
                canvas.scene->removeItem(item);

                delete item;
            }

            canvas.group_list.removeAt(i);

            //QTimer.singleShot(0, Canvas.scene->update);
            return;
        }
    }

    printf("PatchCanvas::removeGroup - Unable to find group to remove\n");
}

void renameGroup(int group_id, const char* new_name)
{
    //if (canvas.debug)
        //printf("PatchCanvas::renameGroup(%i, %s)\n", group_id, new_name);

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].id == group_id)
        {
            canvas.group_list[i].widgets[0]->setText(new_name);
            canvas.group_list[i].name = QString(new_name);

            if (canvas.group_list[i].split)
            {
                canvas.group_list[i].widgets[1]->setText(new_name);
                canvas.group_list[i].name = QString(new_name);
            }

            //QTimer::singleShot(0, Canvas.scene, SLOT());
            return;
        }
    }

    printf("PatchCanvas::renameGroup - Unable to find group to rename\n");
}

END_NAMESPACE_PATCHCANVAS
