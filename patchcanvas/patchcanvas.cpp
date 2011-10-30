#include "patchcanvas.h"

#include "canvasbox.h"
#include "canvasfadeanimation.h"

#include <cstdio>
#include <QSettings>
#include <QTimer>

#ifndef PATCHCANVAS_ORGANISATION_NAME
#define PATCHCANVAS_ORGANISATION_NAME "PatchCanvas"
#endif

TimerObject::TimerObject() {}

void TimerObject::CanvasPostponedGroups()
{
    //PatchCanvas::CanvasPostponedGroups();
}

START_NAMESPACE_PATCHCANVAS

/* Global objects */
Canvas canvas;

options_t options = {
    /* theme_name */         Theme::getThemeName(Theme::getDefaultTheme()),
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
    postponed_timer = 0;
    settings = 0;
    theme = 0;
}

Canvas::~Canvas()
{
    if (postponed_timer)
        delete postponed_timer;
    if (settings)
        delete settings;
    if (theme)
        delete theme;
}

/* Internal functions */
const char* bool2str(bool check)
{
    return check ? "true" : "false";
}

/* PatchCanvas API */
void set_options(options_t* new_options)
{
    options.theme_name = new_options->theme_name;
    options.bezier_lines = new_options->bezier_lines;
    options.antialiasing = new_options->antialiasing;
    options.auto_hide_groups = new_options->auto_hide_groups;
    options.connect_midi2outro = new_options->connect_midi2outro;
    options.fancy_eyecandy = new_options->fancy_eyecandy;
}

void set_features(features_t* new_features)
{
    features.group_rename = new_features->group_rename;
    features.port_rename = new_features->port_rename;
    features.handle_group_pos = new_features->handle_group_pos;
}

void init(QGraphicsScene* scene, void* callback, bool debug)
{
    if (debug)
        printf("PatchCanvas::init(%p, %p, %s)\n", scene, callback, bool2str(debug));

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
    canvas.postponed_timer = new TimerObject();

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

#if 0
    int i;

    for (i=0; i < canvas.connection_list.count();)
        disconnectPorts(canvas.connection_list[0].connection_id);

    for (i=0; i < canvas.port_list.count();)
        removePort(canvas.port_list[0].port_id);

    for (i=0; i < canvas.group_list.count();)
        removeGroup(canvas.group_list[0].group_id);

    for (i=0; i < canvas.animation_list.count();) {
        if (canvas.animation_list[i].animation->state() == QAbstractAnimation::Running) {
            canvas.animation_list[i].animation->stop();
            RemoveItemFX(canvas.animation_list[i].item);
        }
        //delete canvas.animation_list[i].animation;
    }
#endif

    canvas.last_z_value = 0;
    canvas.last_group_id = 0;
    canvas.last_connection_id = 0;

    canvas.group_list.clear();
    canvas.port_list.clear();
    canvas.connection_list.clear();
    canvas.animation_list.clear();
    canvas.postponed_groups.clear(); // TESTING - force remove, or wait for removal?
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
        printf("PatchCanvas::setCanvasSize(%i, %i, %i, %i)\n", x, y, width, height);

    canvas.size_rect = QRectF(x, y, width, height);
}

void addGroup(int group_id, QString group_name, bool split, Icon icon)
{
    if (canvas.debug)
        printf("PatchCanvas::addGroup(%i, %s, %s, %i)\n", group_id, group_name.toStdString().data(), bool2str(split), icon);

    CanvasBox* group_box = new CanvasBox(group_id, group_name, icon);

    group_dict_t group_dict;
    group_dict.group_id = group_id;
    group_dict.group_name = group_name;
    group_dict.split = split;
    group_dict.icon = icon;
    group_dict.widgets[0] = group_box;

    if (split)
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
            group_sbox->setPos(canvas.settings->value(QString("CanvasPositions/%s_i").arg(group_name), CanvasGetNewGroupPos(true)).toPointF());
        else
            group_sbox->setPos(CanvasGetNewGroupPos(true));

        if (!options.auto_hide_groups && options.fancy_eyecandy)
            ItemFX(group_sbox, true);

        canvas.last_z_value += 2;
        group_sbox->setZValue(canvas.last_z_value);
    }
    else
    {
        bool horizontal;

        if (group_name == "Hardware Capture" || group_name == "Hardware Playback" || group_name == "Capture" || group_name == "Playback")
            horizontal = true;
        else
            horizontal = false;

        group_box->setSplit(false);

        if (features.handle_group_pos)
            group_box->setPos(canvas.settings->value(QString("CanvasPositions/%s").arg(group_name), CanvasGetNewGroupPos()).toPointF());
        else
            group_box->setPos(CanvasGetNewGroupPos(horizontal));
    }

    if (!options.auto_hide_groups && options.fancy_eyecandy)
        ItemFX(group_box, true);

    canvas.last_z_value += 1;
    group_box->setZValue(canvas.last_z_value);

    canvas.group_list.append(group_dict);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void removeGroup(int group_id)
{
    if (canvas.debug)
        printf("PatchCanvas::removeGroup(%i)\n", group_id);

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            CanvasBox* item = canvas.group_list[i].widgets[0];
            QString group_name = canvas.group_list[i].group_name;

            if (item->getPortCount() > 0)
            {
                if (canvas.debug)
                    printf("PatchCanvas::removeGroup - This group still has ports, postpone it's removal\n");
                canvas.postponed_groups.append(group_id);
                QTimer::singleShot(100, canvas.postponed_timer, SIGNAL(CanvasPostponedGroups()));
                return;
            }

            if (canvas.group_list[i].split)
            {
                CanvasBox* s_item = canvas.group_list[i].widgets[1];
                if (features.handle_group_pos)
                {
                    canvas.settings->setValue(QString("CanvasPositions/%1_o").arg(group_name), item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_i").arg(group_name), s_item->pos());
                }

                if (options.fancy_eyecandy && s_item->isVisible())
                  ItemFX(s_item, false);
                else
                {
                    item->removeIconFromScene();
                    canvas.scene->removeItem(s_item);
                    delete s_item;
                }
            }
            else
            {
                if (features.handle_group_pos)
                    canvas.settings->setValue(QString("CanvasPositions/%1").arg(group_name), item->pos());
            }

            if (options.fancy_eyecandy && item->isVisible())
              ItemFX(item, false);
            else
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

    printf("PatchCanvas::removeGroup - Unable to find group to remove\n");
}

void renameGroup(int group_id, QString new_name)
{
    if (canvas.debug)
        printf("PatchCanvas::renameGroup(%i, %s)\n", group_id, new_name.toStdString().data());

    for (int i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            canvas.group_list[i].widgets[0]->setText(new_name);
            canvas.group_list[i].group_name = new_name;

            if (canvas.group_list[i].split)
            {
                canvas.group_list[i].widgets[1]->setText(new_name);
                canvas.group_list[i].group_name = new_name;
            }

            QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
            return;
        }
    }

    printf("PatchCanvas::renameGroup - Unable to find group to rename\n");
}

void splitGroup(int group_id)
{
    if (canvas.debug)
        printf("PatchCanvas::splitGroup(%i)\n", group_id);

    int i;
    CanvasBox* item = 0;
    QString group_name;
    Icon group_icon = ICON_APPLICATION;
    QList<port_dict_t> ports_data;
    QList<int> ports_list_ids;
    QList<connection_dict_t> conns_data;

    // Step 1 - Store all Item data
    for (i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            item = canvas.group_list[i].widgets[0];
            group_name = canvas.group_list[i].group_name;
            group_icon = canvas.group_list[i].icon;
            break;
        }
    }

    if (!item)
    {
        printf("PatchCanvas::splitGroup - Unable to find group to split\n");
        return;
    }

    QList<port_dict_t> port_list = item->getPortList();

    for (i=0; i < port_list.count(); i++)
    {
      port_dict_t port_dict;
      port_dict.port_id   = port_list[i].port_id;
      port_dict.port_name = port_list[i].port_name;
      port_dict.port_mode = port_list[i].port_mode;
      port_dict.port_type = port_list[i].port_type;
      port_dict.widget    = 0;

      ports_data.append(port_dict);
      ports_list_ids.append(port_dict.port_id);
    }

    for (i=0; i < canvas.connection_list.count(); i++)
    {
      if (ports_list_ids.contains(canvas.connection_list[i].port_out_id) || ports_list_ids.contains(canvas.connection_list[i].port_in_id))
          conns_data.append(canvas.connection_list[i]);
    }

    // Step 2 - Remove Item and Children
    for (i=0; i < conns_data.count(); i++)
        disconnectPorts(conns_data[i].connection_id);

    for (i=0; i < ports_list_ids.count(); i++)
        removePort(ports_list_ids[i]);

    removeGroup(group_id);

    // Step 3 - Re-create Item, now splitted
    addGroup(group_id, group_name, true, group_icon);

    for (i=0; i < ports_data.count(); i++)
        addPort(group_id, ports_data[i].port_id, ports_data[i].port_name, ports_data[i].port_mode, ports_data[i].port_type);

    for (i=0; i < conns_data.count(); i++)
      connectPorts(conns_data[i].connection_id, conns_data[i].port_out_id, conns_data[i].port_in_id);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void joinGroup(int group_id)
{
    if (canvas.debug)
        printf("PatchCanvas::joinGroup(%i)\n", group_id);

    int i;
    CanvasBox* item = 0;
    CanvasBox* s_item = 0;
    QString group_name;
    Icon group_icon = ICON_APPLICATION;
    QList<port_dict_t> ports_data;
    QList<int> ports_list_ids;
    QList<connection_dict_t> conns_data;

    // Step 1 - Store all Item data
    for (i=0; i < canvas.group_list.count(); i++)
    {
        if (canvas.group_list[i].group_id == group_id)
        {
            item = canvas.group_list[i].widgets[0];
            s_item = canvas.group_list[i].widgets[1];
            group_name = canvas.group_list[i].group_name;
            group_icon = canvas.group_list[i].icon;
            break;
        }
    }

    if (!item || !s_item)
    {
        printf("PatchCanvas::joinGroup - Unable to find groups to join\n");
        return;
    }

    QList<port_dict_t> port_list;
    port_list = item->getPortList();

    for (i=0; i < port_list.count(); i++)
    {
      port_dict_t port_dict;
      port_dict.port_id   = port_list[i].port_id;
      port_dict.port_name = port_list[i].port_name;
      port_dict.port_mode = port_list[i].port_mode;
      port_dict.port_type = port_list[i].port_type;
      port_dict.widget    = 0;

      ports_data.append(port_dict);
      ports_list_ids.append(port_dict.port_id);
    }

    port_list = s_item->getPortList();

    for (i=0; i < port_list.count(); i++)
    {
      port_dict_t port_dict;
      port_dict.port_id   = port_list[i].port_id;
      port_dict.port_name = port_list[i].port_name;
      port_dict.port_mode = port_list[i].port_mode;
      port_dict.port_type = port_list[i].port_type;
      port_dict.widget    = 0;

      ports_data.append(port_dict);
      ports_list_ids.append(port_dict.port_id);
    }

    for (i=0; i < canvas.connection_list.count(); i++)
    {
      if (ports_list_ids.contains(canvas.connection_list[i].port_out_id) || ports_list_ids.contains(canvas.connection_list[i].port_in_id))
          conns_data.append(canvas.connection_list[i]);
    }

    // Step 2 - Remove Item and Children
    for (i=0; i < conns_data.count(); i++)
        disconnectPorts(conns_data[i].connection_id);

    for (i=0; i < ports_list_ids.count(); i++)
        removePort(ports_list_ids[i]);

    removeGroup(group_id);

    // Step 3 - Re-create Item, now together
    addGroup(group_id, group_name, false, group_icon);

    for (i=0; i < ports_data.count(); i++)
        addPort(group_id, ports_data[i].port_id, ports_data[i].port_name, ports_data[i].port_mode, ports_data[i].port_type);

    for (i=0; i < conns_data.count(); i++)
      connectPorts(conns_data[i].connection_id, conns_data[i].port_out_id, conns_data[i].port_in_id);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void setGroupPos(int group_id, int group_pos_x, int group_pos_y, int group_pos_xs, int group_pos_ys)
{
    if (canvas.debug)
      printf("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i)\n", group_id, group_pos_x, group_pos_y, group_pos_xs, group_pos_ys);

    bool check = false;

    for (int i=0; i < canvas.group_list.count(); i++)
    {
      if (group_id == canvas.group_list[i].group_id)
      {
        canvas.group_list[i].widgets[0]->setPos(group_pos_x, group_pos_y);

        if (canvas.group_list[i].split)
          canvas.group_list[i].widgets[1]->setPos(group_pos_xs, group_pos_ys);

        QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
        break;
      }
    }

    printf("PatchCanvas::setGroupPos - Unable to find group to reposition\n");
}

void setGroupIcon(int group_id, Icon icon)
{
    if (canvas.debug)
      printf("PatchCanvas::setGroupIcon(%i, %i)\n", group_id, icon);

    for (int i=0; i < canvas.group_list.count(); i++)
    {
      if (group_id == canvas.group_list[i].group_id)
      {
        canvas.group_list[i].icon = icon;
        canvas.group_list[i].widgets[0]->setIcon(icon);

        if (canvas.group_list[i].split)
          canvas.group_list[i].widgets[1]->setIcon(icon);

        QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
        break;
      }
    }

    printf("PatchCanvas::setGroupIcon - Unable to find group to change icon\n");
}

void addPort(int group_id, int port_id, QString port_name, PortMode port_mode, PortType port_type)
{
    if (canvas.debug)
      printf("PatchCanvas::addPort(%i, %i, %s, %i, %i)\n", group_id, port_id, port_name.toStdString().data(), port_mode, port_type);

    port_dict_t new_port;

    for (int i=0; i < canvas.group_list.count(); i++)
    {
      if (group_id == canvas.group_list[i].group_id)
      {
          if (canvas.group_list[i].split)
          {
              int n_widget;
              if (canvas.group_list[i].widgets[0]->getSplittedMode() == port_mode)
                  n_widget = 0;
              else
                  n_widget = 1;
              new_port = canvas.group_list[i].widgets[n_widget]->addPort(port_id, port_name, port_mode, port_type);
          }
          else
              new_port = canvas.group_list[i].widgets[0]->addPort(port_id, port_name, port_mode, port_type);

          break;
      }
    }

    if (!new_port.widget)
    {
        printf("PatchCanvas::addPort - Unable to find parent group\n");
        return;
    }

    //if (options.fancy_eyecandy)
    //  ItemFX(new_port.widget, true);

    canvas.port_list.append(new_port);

    QTimer::singleShot(0, canvas.scene, SIGNAL(update()));
}

void removePort(int port_id)
{

}

void renamePort(int port_id, QString new_port_name)
{

}

void connectPorts(int connection_id, int port_out_id, int port_in_id)
{

}

void disconnectPorts(int connection_id)
{

}

void Arrange()
{

}

/* Extra Internal functions */

QPointF CanvasGetNewGroupPos(bool horizontal)
{
    if (canvas.debug)
      printf("PatchCanvas::CanvasGetNewGroupPos(%s)\n", bool2str(horizontal));

    QPointF new_pos(canvas.initial_pos.x(), canvas.initial_pos.y());
    QList<QGraphicsItem*> items = canvas.scene->items();

    while (true)
    {
        for (int i=0; i < items.count(); i++)
        {
            if (items[i]->flags() == (QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable) /* CanvasBox */)
            {
                //          #item_rect = QRectF(items[i].scenePos().x(), items[i].scenePos().y(), items[i].boundingRect().width(), items[i].boundingRect().height())
                //          #item_rect = items[i].sceneBoundingRect()
                if (items[i]->sceneBoundingRect().contains(new_pos))
                {
                    if (horizontal)
                        new_pos += QPointF(items[i]->boundingRect().width()+15, 0);
                    else
                        new_pos += QPointF(0, items[i]->boundingRect().height()+15);
                    break;
                }
            }
            //      else:
        }
        break;
    }

    return new_pos;
}

void CanvasPostponedGroups()
{
    if (canvas.debug)
        printf("PatchCanvas::CanvasPostponedGroups()\n");

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
        QTimer::singleShot(100, canvas.postponed_timer, SIGNAL(CanvasPostponedGroups()));
}

void ItemFX(QGraphicsItem* item, bool show, bool destroy)
{

}

void RemoveItemFX(QGraphicsItem* item)
{

}

END_NAMESPACE_PATCHCANVAS
