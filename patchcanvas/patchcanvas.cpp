#include "patchcanvas.h"

#include "canvasbox.h"
#include "canvasport.h"
#include "canvasline.h"
#include "canvasbezierline.h"
#include "canvasfadeanimation.h"
#include "patchscene.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QAction>

#ifndef PATCHCANVAS_ORGANISATION_NAME
#define PATCHCANVAS_ORGANISATION_NAME "PatchCanvas"
#endif

CanvasObject::CanvasObject(QObject* parent) : QObject(parent) {}

void CanvasObject::CanvasPostponedGroups()
{
    PatchCanvas::CanvasPostponedGroups();
}

void CanvasObject::PortContextMenuDisconnect()
{
    bool ok;
    int connection_id = ((QAction*)sender())->data().toInt(&ok);
    if (ok)
        PatchCanvas::CanvasCallback(PatchCanvas::ACTION_PORTS_DISCONNECT, connection_id, 0, "");
}

START_NAMESPACE_PATCHCANVAS

/* contructor and destructor */
Canvas::Canvas()
{
    qobject = 0;
    settings = 0;
    theme = 0;
    initiated = false;
}

Canvas::~Canvas()
{
    if (qobject)
        delete qobject;
    if (settings)
        delete settings;
    if (theme)
        delete theme;
}

/* Global objects */
Canvas canvas;

options_t options = {
    /* theme_name */       Theme::getThemeName(Theme::getDefaultTheme()),
    /* bezier_lines */     true,
    /* antialiasing */     Qt::PartiallyChecked,
    /* auto_hide_groups */ true,
    /* fancy_eyecandy */   false
};

features_t features = {
    /* group_info */       false,
    /* group_rename */     true,
    /* port_info */        false,
    /* port_rename */      true,
    /* handle_group_pos */ false
};

/* Internal functions */
const char* bool2str(bool check)
{
    return check ? "true" : "false";
}

/* PatchCanvas API */
void set_options(options_t* new_options)
{
    if (canvas.initiated) return;
    options.theme_name        = new_options->theme_name;
    options.bezier_lines      = new_options->bezier_lines;
    options.antialiasing      = new_options->antialiasing;
    options.auto_hide_groups  = new_options->auto_hide_groups;
    options.fancy_eyecandy    = new_options->fancy_eyecandy;
}

void set_features(features_t* new_features)
{
    if (canvas.initiated) return;
    features.group_info       = new_features->group_info;
    features.group_rename     = new_features->group_rename;
    features.port_info        = new_features->port_info;
    features.port_rename      = new_features->port_rename;
    features.handle_group_pos = new_features->handle_group_pos;
}

void init(PatchScene* scene, Callback callback, bool debug)
{
    if (debug)
        qDebug("PatchCanvas::init(%p, %p, %s)", scene, callback, bool2str(debug));

    if (canvas.initiated)
    {
        qCritical("PatchCanvas::init() - already initiated");
        return;
    }

    if (!callback)
    {
        qFatal("PatchCanvas::init() - fatal error: callback not set");
        return;
    }

    canvas.scene = scene;
    canvas.callback = callback;
    canvas.debug = debug;

    canvas.last_z_value = 0;
    canvas.last_group_id = 0;
    canvas.last_connection_id = 0;
    canvas.initial_pos = QPointF(0, 0);

    canvas.group_list.clear();
    canvas.port_list.clear();
    canvas.connection_list.clear();
    canvas.animation_list.clear();

    canvas.postponed_groups.clear();
    if (!canvas.qobject) canvas.qobject = new CanvasObject();
    if (!canvas.settings) canvas.settings = new QSettings(PATCHCANVAS_ORGANISATION_NAME, "PatchCanvas");

    if (canvas.theme)
    {
        delete canvas.theme;
        canvas.theme = 0;
    }

    for (int i=0; i<Theme::THEME_MAX; i++)
    {
        QString this_theme_name = Theme::getThemeName(static_cast<Theme::List>(i));
        if (this_theme_name == options.theme_name)
        {
            canvas.theme = new Theme(static_cast<Theme::List>(i));
            break;
        }
    }

    if (!canvas.theme)
        canvas.theme = new Theme(Theme::getDefaultTheme());

    canvas.size_rect = QRectF();

    ((PatchScene*)canvas.scene)->rubberbandByTheme();
    canvas.scene->setBackgroundBrush(canvas.theme->canvas_bg);

    canvas.initiated = true;
}

void clear()
{
    if (canvas.debug)
        qDebug("PatchCanvas::clear()");

    int i;
    QList<int> tmp_group_list;
    QList<int> tmp_port_list;
    QList<int> tmp_connection_list;

    for (i=0; i < canvas.group_list.count(); i++)
        tmp_group_list.append(canvas.group_list[i].group_id);

    for (i=0; i < canvas.port_list.count(); i++)
        tmp_port_list.append(canvas.port_list[i].port_id);

    for (i=0; i < canvas.connection_list.count(); i++)
        tmp_connection_list.append(canvas.connection_list[i].connection_id);

    //for (i=0; i < canvas.animation_list.count(); i++) {
        //if (canvas.animation_list[i].animation->state() == QAbstractAnimation::Running) {
            //canvas.animation_list[i].animation->stop();
            //RemoveItemFX(canvas.animation_list[i].item);
        //}
        //delete canvas.animation_list[i].animation;
    //}

    canvas.animation_list.clear();

    for (i=0; i < tmp_connection_list.count(); i++)
        disconnectPorts(tmp_connection_list[i]);

    for (i=0; i < tmp_port_list.count(); i++)
        removePort(tmp_port_list[i]);

    for (i=0; i < tmp_group_list.count(); i++)
        removeGroup(tmp_group_list[i]);

    canvas.last_z_value = 0;
    canvas.last_group_id = 0;
    canvas.last_connection_id = 0;

    canvas.group_list.clear();
    canvas.port_list.clear();
    canvas.connection_list.clear();
    canvas.postponed_groups.clear();

    canvas.initiated = false;
}

void setInitialPos(int x, int y)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setInitialPos(%i, %i)", x, y);

    canvas.initial_pos.setX(x);
    canvas.initial_pos.setY(y);
}

void setCanvasSize(int x, int y, int width, int height)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setCanvasSize(%i, %i, %i, %i)", x, y, width, height);

    canvas.size_rect.setX(x);
    canvas.size_rect.setY(y);
    canvas.size_rect.setWidth(width);
    canvas.size_rect.setHeight(height);
}

void addGroup(int group_id, QString group_name, SplitOption split, Icon icon)
{
    if (canvas.debug)
        qDebug("PatchCanvas::addGroup(%i, %s, %i, %i)", group_id, group_name.toStdString().data(), split, icon);

    if (split == SPLIT_UNDEF && features.handle_group_pos)
        split = static_cast<SplitOption>(canvas.settings->value(QString("CanvasPositions/%1_s").arg(group_name), split).toInt());

    CanvasBox* group_box = new CanvasBox(group_id, group_name, icon);

    group_dict_t group_dict;
    group_dict.group_id   = group_id;
    group_dict.group_name = group_name;
    group_dict.split = (split == SPLIT_YES);
    group_dict.icon  = icon;
    group_dict.widgets[0] = group_box;
    group_dict.widgets[1] = 0;

    if (split == SPLIT_YES)
    {
        group_box->setSplit(true, PORT_MODE_OUTPUT);

        if (features.handle_group_pos)
            group_box->setPos(canvas.settings->value(QString("CanvasPositions/%1_o").arg(group_name), CanvasGetNewGroupPos()).toPointF());
        else
            group_box->setPos(CanvasGetNewGroupPos());

        CanvasBox* group_sbox = new CanvasBox(group_id, group_name, icon);
        group_sbox->setSplit(true, PORT_MODE_INPUT);

        group_dict.widgets[1] = group_sbox;

        if (features.handle_group_pos)
            group_sbox->setPos(canvas.settings->value(QString("CanvasPositions/%1_i").arg(group_name), CanvasGetNewGroupPos(true)).toPointF());
        else
            group_sbox->setPos(CanvasGetNewGroupPos(true));

        //if (!options.auto_hide_groups && options.fancy_eyecandy)
            //ItemFX(group_sbox, true);

        canvas.last_z_value += 1;
        group_sbox->setZValue(canvas.last_z_value);
    }
    else
    {
        group_box->setSplit(false);

        if (features.handle_group_pos)
            group_box->setPos(canvas.settings->value(QString("CanvasPositions/%1").arg(group_name), CanvasGetNewGroupPos()).toPointF());
        else
        {
            // Special ladish fake-split groups
            bool horizontal = (icon == ICON_HARDWARE || icon == ICON_LADISH_ROOM);
            group_box->setPos(CanvasGetNewGroupPos(horizontal));
        }
    }

    //if (!options.auto_hide_groups && options.fancy_eyecandy)
        //ItemFX(group_box, true);

    canvas.last_z_value += 1;
    group_box->setZValue(canvas.last_z_value);

    canvas.group_list.append(group_dict);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void removeGroup(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::removeGroup(%i)", group_id);

    if (CanvasGetGroupPortCount(group_id) > 0)
    {
        if (canvas.debug)
            qDebug("PatchCanvas::removeGroup() - This group still has ports, postpone it's removal");
        canvas.postponed_groups.append(group_id);
        QTimer::singleShot(100, canvas.qobject, SLOT(CanvasPostponedGroups()));
        return;
    }

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            CanvasBox* item = canvas.group_list[i].widgets[0];
            QString group_name = canvas.group_list[i].group_name;

            if (canvas.group_list[i].split)
            {
                CanvasBox* s_item = canvas.group_list[i].widgets[1];
                if (features.handle_group_pos)
                {
                    canvas.settings->setValue(QString("CanvasPositions/%1_o").arg(group_name), item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_i").arg(group_name), s_item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_s").arg(group_name), SPLIT_YES);
                }

                //if (options.fancy_eyecandy && s_item->isVisible())
                    //ItemFX(s_item, false);
                //else
                {
                    item->removeIconFromScene();
                    canvas.scene->removeItem(s_item);
                    delete s_item;
                }
            }
            else
            {
                if (features.handle_group_pos)
                {
                    canvas.settings->setValue(QString("CanvasPositions/%1").arg(group_name), item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_s").arg(group_name), SPLIT_NO);
                }
            }

            //if (options.fancy_eyecandy && item->isVisible())
                //ItemFX(item, false);
            //else
            {
                item->removeIconFromScene();
                canvas.scene->removeItem(item);
                delete item;
            }

            canvas.group_list.removeAt(i);

            QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
            return;
        }
    }

    qCritical("PatchCanvas::removeGroup() - Unable to find group to remove");
}

void renameGroup(int group_id, QString new_group_name)
{
    if (canvas.debug)
        qDebug("PatchCanvas::renameGroup(%i, %s)", group_id, new_group_name.toStdString().data());

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            canvas.group_list[i].widgets[0]->setGroupName(new_group_name);
            canvas.group_list[i].group_name = new_group_name;

            if (canvas.group_list[i].split)
            {
                canvas.group_list[i].widgets[1]->setGroupName(new_group_name);
            }

            QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
            return;
        }
    }

    qCritical("PatchCanvas::renameGroup() - Unable to find group to rename");
}

void splitGroup(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::splitGroup(%i)", group_id);

    int i;
    CanvasBox* item = 0;
    QString group_name;
    Icon group_icon = ICON_APPLICATION;
    QList<port_dict_t> ports_data;
    QList<connection_dict_t> conns_data;

    // Step 1 - Store all Item data
    for (i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            if (canvas.group_list[i].split)
            {
                qCritical("PatchCanvas::splitGroup() - group is already splitted");
                return;
            }

            item = canvas.group_list[i].widgets[0];
            group_name = canvas.group_list[i].group_name;
            group_icon = canvas.group_list[i].icon;
            break;
        }
    }

    if (!item)
    {
        qCritical("PatchCanvas::splitGroup() - Unable to find group to split");
        return;
    }

    QList<int> port_list_ids = item->getPortList();

    for (i=0; i < canvas.port_list.count(); i++)
    {
        if (port_list_ids.contains(canvas.port_list[i].port_id))
        {
            port_dict_t port_dict;
            port_dict.group_id  = canvas.port_list[i].group_id;
            port_dict.port_id   = canvas.port_list[i].port_id;
            port_dict.port_name = canvas.port_list[i].port_name;
            port_dict.port_mode = canvas.port_list[i].port_mode;
            port_dict.port_type = canvas.port_list[i].port_type;
            port_dict.widget    = 0;
            ports_data.append(port_dict);
        }
    }

    for (i=0; i < canvas.connection_list.count(); i++)
    {
        if (port_list_ids.contains(canvas.connection_list[i].port_out_id) || port_list_ids.contains(canvas.connection_list[i].port_in_id))
            conns_data.append(canvas.connection_list[i]);
    }

    // Step 2 - Remove Item and Children
    for (i=0; i < conns_data.count(); i++)
        disconnectPorts(conns_data[i].connection_id);

    for (i=0; i < port_list_ids.count(); i++)
        removePort(port_list_ids[i]);

    removeGroup(group_id);

    // Step 3 - Re-create Item, now splitted
    addGroup(group_id, group_name, SPLIT_YES, group_icon);

    for (i=0; i < ports_data.count(); i++)
        addPort(group_id, ports_data[i].port_id, ports_data[i].port_name, ports_data[i].port_mode, ports_data[i].port_type);

    for (i=0; i < conns_data.count(); i++)
        connectPorts(conns_data[i].connection_id, conns_data[i].port_out_id, conns_data[i].port_in_id);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void joinGroup(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::joinGroup(%i)", group_id);

    int i;
    CanvasBox* item = 0;
    CanvasBox* s_item = 0;
    QString group_name;
    Icon group_icon = ICON_APPLICATION;
    QList<port_dict_t> ports_data;
    QList<connection_dict_t> conns_data;

    // Step 1 - Store all Item data
    for (i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            if (!canvas.group_list[i].split)
            {
                qCritical("PatchCanvas::joinGroup() - group is not splitted");
                return;
            }

            item   = canvas.group_list[i].widgets[0];
            s_item = canvas.group_list[i].widgets[1];
            group_name = canvas.group_list[i].group_name;
            group_icon = canvas.group_list[i].icon;
            break;
        }
    }

    if (!item || !s_item)
    {
        qCritical("PatchCanvas::joinGroup() - Unable to find groups to join");
        return;
    }

    QList<int> port_list_ids  = item->getPortList();
    QList<int> port_list_idss = s_item->getPortList();
     for (i=0; i < port_list_idss.count(); i++)
    {
      if (!port_list_ids.contains(port_list_idss[i]))
        port_list_ids.append(port_list_idss[i]);
    }

     for (i=0; i < canvas.port_list.count(); i++)
     {
         if (port_list_ids.contains(canvas.port_list[i].port_id))
         {
             port_dict_t port_dict;
             port_dict.group_id  = canvas.port_list[i].group_id;
             port_dict.port_id   = canvas.port_list[i].port_id;
             port_dict.port_name = canvas.port_list[i].port_name;
             port_dict.port_mode = canvas.port_list[i].port_mode;
             port_dict.port_type = canvas.port_list[i].port_type;
             port_dict.widget    = 0;
             ports_data.append(port_dict);
         }
     }

    for (i=0; i < canvas.connection_list.count(); i++)
    {
        if (port_list_ids.contains(canvas.connection_list[i].port_out_id) || port_list_ids.contains(canvas.connection_list[i].port_in_id))
            conns_data.append(canvas.connection_list[i]);
    }

    // Step 2 - Remove Item and Children
    for (i=0; i < conns_data.count(); i++)
        disconnectPorts(conns_data[i].connection_id);

    for (i=0; i < port_list_ids.count(); i++)
        removePort(port_list_ids[i]);

    removeGroup(group_id);

    // Step 3 - Re-create Item, now together
    addGroup(group_id, group_name, SPLIT_NO, group_icon);

    for (i=0; i < ports_data.count(); i++)
        addPort(group_id, ports_data[i].port_id, ports_data[i].port_name, ports_data[i].port_mode, ports_data[i].port_type);

    for (i=0; i < conns_data.count(); i++)
        connectPorts(conns_data[i].connection_id, conns_data[i].port_out_id, conns_data[i].port_in_id);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void setGroupPos(int group_id, int group_pos_x, int group_pos_y)
{
    setGroupPos(group_id, group_pos_x, group_pos_y, group_pos_x, group_pos_y);
}

void setGroupPos(int group_id, int group_pos_x, int group_pos_y, int group_pos_xs, int group_pos_ys)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i)", group_id, group_pos_x, group_pos_y, group_pos_xs, group_pos_ys);

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            canvas.group_list[i].widgets[0]->setPos(group_pos_x, group_pos_y);

            if (canvas.group_list[i].split)
            {
                canvas.group_list[i].widgets[1]->setPos(group_pos_xs, group_pos_ys);
            }

            QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
            return;
        }
    }

    qCritical("PatchCanvas::setGroupPos() - Unable to find group to reposition");
}

void setGroupIcon(int group_id, Icon icon)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setGroupIcon(%i, %i)", group_id, icon);

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            canvas.group_list[i].icon = icon;
            canvas.group_list[i].widgets[0]->setIcon(icon);

            if (canvas.group_list[i].split)
                canvas.group_list[i].widgets[1]->setIcon(icon);

            QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
            return;
        }
    }

    qCritical("PatchCanvas::setGroupIcon() - Unable to find group to change icon");
}

void addPort(int group_id, int port_id, QString port_name, PortMode port_mode, PortType port_type)
{
    if (canvas.debug)
        qDebug("PatchCanvas::addPort(%i, %i, %s, %i, %i)", group_id, port_id, port_name.toStdString().data(), port_mode, port_type);

    CanvasBox* box_widget = 0;
    CanvasPort* port_widget = 0;

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            int n_widget = 0;
            if (canvas.group_list[i].split && canvas.group_list[i].widgets[0]->getSplittedMode() != port_mode)
                n_widget = 1;
            box_widget = canvas.group_list[i].widgets[n_widget];
            port_widget = box_widget->addPortFromGroup(port_id, port_name, port_mode, port_type);
            break;
        }
    }

    if (!box_widget || !port_widget)
    {
        qCritical("PatchCanvas::addPort() - Unable to find parent group");
        return;
    }

    //if (options.fancy_eyecandy)
        //ItemFX(port_widget, true);

    port_dict_t port_dict;
    port_dict.group_id  = group_id;
    port_dict.port_id   = port_id;
    port_dict.port_name = port_name;
    port_dict.port_mode = port_mode;
    port_dict.port_type = port_type;
    port_dict.widget    = port_widget;
    canvas.port_list.append(port_dict);

    box_widget->updatePositions();

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void removePort(int port_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::removePort(%i)", port_id);

    for (int i=0; i < canvas.port_list.count(); i++)
    {
        if (canvas.port_list[i].port_id == port_id)
        {
            CanvasPort* item = canvas.port_list[i].widget;
            ((CanvasBox*)item->parentItem())->removePortFromGroup(port_id);
            //if (options.fancy_eyecandy)
                //ItemFX(item, false, true);
            /*else*/ {
                canvas.scene->removeItem(item);
                delete item;
            }
            canvas.port_list.takeAt(i);

            QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
            return;
        }
    }

    qCritical("PatchCanvas::removePort() - Unable to find port to remove");
}

void renamePort(int port_id, QString new_port_name)
{
    if (canvas.debug)
        qDebug("PatchCanvas::renamePort(%i, %s)", port_id, new_port_name.toStdString().data());

    for (int i=0; i < canvas.port_list.count(); i++)
    {
        if (canvas.port_list[i].port_id == port_id)
        {
            canvas.port_list[i].port_name = new_port_name;
            canvas.port_list[i].widget->setPortName(new_port_name);
            ((CanvasBox*)canvas.port_list[i].widget->parentItem())->updatePositions();

            QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
            return;
        }
    }

    qCritical("PatchCanvas::renamePort() - Unable to find port to rename");
}

void connectPorts(int connection_id, int port_out_id, int port_in_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::connectPorts(%i, %i, %i)", connection_id, port_out_id, port_in_id);

    CanvasPort* port_out = 0;
    CanvasPort* port_in  = 0;
    CanvasBox* port_out_parent = 0;
    CanvasBox* port_in_parent  = 0;

    for (int i=0; i < canvas.port_list.count(); i++)
    {
        if (canvas.port_list[i].port_id == port_out_id)
        {
            port_out = canvas.port_list[i].widget;
            port_out_parent = (CanvasBox*)port_out->parentItem();
        }
        else if (canvas.port_list[i].port_id == port_in_id)
        {
            port_in = canvas.port_list[i].widget;
            port_in_parent = (CanvasBox*)port_in->parentItem();
        }
    }

    if (!port_out || !port_in)
    {
        qCritical("PatchCanvas::connectPorts() - Unable to find ports to connect");
        return;
    }

    connection_dict_t connection_dict;
    connection_dict.connection_id = connection_id;
    connection_dict.port_out_id = port_out_id;
    connection_dict.port_in_id  = port_in_id;

    if (options.bezier_lines)
        connection_dict.widget = new CanvasBezierLine(port_out, port_in, 0);
    else
        connection_dict.widget = new CanvasLine(port_out, port_in, 0);

    port_out_parent->addLineFromGroup(connection_dict.widget, connection_id);
    port_in_parent->addLineFromGroup(connection_dict.widget, connection_id);

    canvas.last_z_value += 1;
    port_out_parent->setZValue(canvas.last_z_value);
    port_in_parent->setZValue(canvas.last_z_value);

    canvas.last_z_value += 1;
    connection_dict.widget->setZValue(canvas.last_z_value);

    canvas.connection_list.append(connection_dict);

    //if (options.fancy_eyecandy)
        //ItemFX(connection_dict.widget, true);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void disconnectPorts(int connection_id)
{
    if (canvas.debug)
      qDebug("PatchCanvas::disconnectPorts(%i)", connection_id);

    int port_1_id, port_2_id;
    QGraphicsItem* line = 0;
    QGraphicsItem* item1 = 0;
    QGraphicsItem* item2 = 0;

    for (int i=0; i < canvas.connection_list.count(); i++)
    {
        if (canvas.connection_list[i].connection_id == connection_id)
        {
            port_1_id = canvas.connection_list[i].port_out_id;
            port_2_id = canvas.connection_list[i].port_in_id;
            line = canvas.connection_list[i].widget;
            canvas.connection_list.takeAt(i);
            break;
        }
    }

    if (!line)
    {
        qCritical("PatchCanvas::disconnectPorts - Unable to find connection ports");
        return;
    }

    for (int i=0; i < canvas.port_list.count(); i++)
    {
        if (canvas.port_list[i].port_id == port_1_id)
        {
            item1 = canvas.port_list[i].widget;
            break;
        }
    }

    if (!item1)
    {
        qCritical("PatchCanvas::disconnectPorts - Unable to find output port");
        return;
    }

    for (int i=0; i < canvas.port_list.count(); i++)
    {
        if (canvas.port_list[i].port_id == port_2_id)
        {
            item2 = canvas.port_list[i].widget;
            break;
        }
    }

    if (!item2)
    {
        qCritical("PatchCanvas::disconnectPorts - Unable to find input port");
        return;
    }

    ((CanvasBox*)item1->parentItem())->removeLineFromGroup(connection_id);
    ((CanvasBox*)item2->parentItem())->removeLineFromGroup(connection_id);

    //if (options.fancy_eyecandy)
        //ItemFX(line, false, true);
    //else
    {
        canvas.scene->removeItem(line);
        delete line;
    }

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void Arrange()
{

}

/* Extra Internal functions */

QString CanvasGetGroupName(int group_id)
{
    if (canvas.debug)
      qDebug("PatchCanvas::CanvasGetGroupName(%i)", group_id);

    for (int i=0; i<canvas.group_list.count(); i++)
    {
        if (canvas.port_list[i].group_id == group_id)
            return canvas.group_list[i].group_name;
    }

    qCritical("PatchCanvas::CanvasGetGroupName() - unable to find group");
    return "";
}

int CanvasGetGroupPortCount(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetGroupPortCount(%i)", group_id);

    int port_count = 0;
    for (int i=0; i < canvas.port_list.count(); i++)
    {
        if (canvas.port_list[i].group_id == group_id)
            port_count += 1;
    }

    return port_count;
}

QPointF CanvasGetNewGroupPos(bool horizontal)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetNewGroupPos(%s)", bool2str(horizontal));

    QPointF new_pos(canvas.initial_pos.x(), canvas.initial_pos.y());
    QList<QGraphicsItem*> items = canvas.scene->items();

    bool break_loop = false;
    while (!break_loop)
    {
        bool break_for = false;
        for (int i=0; i < items.count(); i++)
        {
            if (items[i]->type() == CanvasBoxType)
            {
                if (items[i]->sceneBoundingRect().contains(new_pos))
                {
                    if (horizontal)
                        new_pos += QPointF(items[i]->boundingRect().width()+15, 0);
                    else
                        new_pos += QPointF(0, items[i]->boundingRect().height()+15);
                    break;
                }
            }
            if (i >= items.count()-1 && !break_for)
                break_loop = true;
        }
    }

    return new_pos;
}

QString CanvasGetPortName(int port_id)
{
    if (canvas.debug)
      qDebug("PatchCanvas::CanvasGetPortName(%i)", port_id);

    for (int i=0; i<canvas.port_list.count(); i++)
    {
        if (canvas.port_list[i].port_id == port_id)
        {
            int group_id = canvas.port_list[i].group_id;
            for (int j=0; j<canvas.group_list.count(); j++)
            {
                if (canvas.group_list[j].group_id == group_id)
                    return canvas.group_list[j].group_name + ":" + canvas.port_list[i].port_name;

            }
            break;
        }
    }

    qCritical("PatchCanvas::CanvasGetPortName() - unable to find port");
    return "";
}

QList<int> CanvasGetPortConnectionList(int port_id)
{
    if (canvas.debug)
      qDebug("PatchCanvas::CanvasGetPortConnectionList(%i)", port_id);

    QList<int> port_con_list;

    for (int i=0; i<canvas.connection_list.count(); i++)
    {
        if (canvas.connection_list[i].port_out_id == port_id || canvas.connection_list[i].port_in_id == port_id)
            port_con_list.append(canvas.connection_list[i].connection_id);
    }

    return port_con_list;
}

int CanvasGetConnectedPort(int connection_id, int port_id)
{
    if (canvas.debug)
      qDebug("PatchCanvas::CanvasGetConnectedPort(%i, %i)", connection_id, port_id);

    for (int i=0; i<canvas.connection_list.count(); i++)
    {
        if (canvas.connection_list[i].connection_id == connection_id)
        {
            if (canvas.connection_list[i].port_out_id == port_id)
                return canvas.connection_list[i].port_in_id;
            else
                return canvas.connection_list[i].port_out_id;
        }
    }

    qCritical("PatchCanvas::CanvasGetConnectedPort() - unable to find connection");
    return 0;
}

void CanvasPostponedGroups()
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasPostponedGroups()");

    for (int i=0; i < canvas.postponed_groups.count(); i++)
    {
        int group_id = canvas.postponed_groups[i];

        for (int j=0; j < canvas.group_list.count(); j++)
        {
            if (canvas.group_list[j].group_id == group_id)
            {
                CanvasBox* item = canvas.group_list[j].widgets[0];
                CanvasBox* s_item = 0;

                if (canvas.group_list[j].split)
                    s_item = canvas.group_list[j].widgets[1];

                if (item->getPortCount() == 0 && (!s_item || s_item->getPortCount() == 0))
                {
                    removeGroup(group_id);
                    canvas.postponed_groups.takeAt(i);
                }

                break;
            }
        }
    }

    if (canvas.postponed_groups.count() > 0)
        QTimer::singleShot(100, canvas.qobject, SLOT(CanvasPostponedGroups()));
}

void CanvasCallback(CallbackAction action, int value1, int value2, QString value_str)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasCallback(%i, %i, %i, %s)", action, value1, value2, value_str.toStdString().data());

    canvas.callback(action, value1, value2, value_str);
}

void ItemFX(QGraphicsItem* item, bool show, bool destroy)
{
    if (canvas.debug)
        qDebug("patchcanvas::ItemFX(%p, %s, %s)", item, bool2str(show), bool2str(destroy));

    // Check if item already has an animationItemFX
    for (int i=0; i < canvas.animation_list.count(); i++)
    {
        if (canvas.animation_list[i].item == item)
        {
            canvas.animation_list[i].animation->stop();
            canvas.animation_list.takeAt(i);
            break;
        }
    }

    CanvasFadeAnimation* animation = new CanvasFadeAnimation(item, show);
    animation->setDuration(show ? 750 : 500);
    animation->start();

    animation_dict_t animation_dict;
    animation_dict.animation = animation;
    animation_dict.item = item;
    canvas.animation_list.append(animation_dict);

    if (!show)
    {
        //if (destroy)
        //QObject::connect(animation, SIGNAL(finished()), lambda item_=item: RemoveItemFX(item_));
        //else
        //QObject::connect(animation, SIGNAL(finished()), item, SLOT(hide()));
    }
}

void RemoveItemFX(QGraphicsItem* item)
{
    if (canvas.debug)
      qDebug("PatchCanvas::RemoveItemFX(%p)", item);

    if (item->type() == CanvasBoxType)
      ((CanvasBox*)item)->removeIconFromScene();

    canvas.scene->removeItem(item);
}

END_NAMESPACE_PATCHCANVAS
