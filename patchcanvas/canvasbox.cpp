#include "canvasbox.h"
#include "canvasboxshadow.h"
#include "canvasicon.h"
#include "canvasline.h"
#include "canvasbezierline.h"
#include "canvasport.h"

#include <QCursor>
#include <QFontMetrics>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;
extern options_t options;

CanvasBox::CanvasBox(int group_id_, QString group_name_, Icon icon, QGraphicsItem* parent) :
    QGraphicsItem(parent, canvas.scene)
{
    // Save Variables, useful for later
    group_id = group_id_;
    group_name = group_name_;

    // Base Variables
    box_width  = 50;
    box_height = 25;
    port_list.clear();
    port_list_ids.clear();
    connection_lines.clear();

    last_pos = QPointF();
    splitted = false;
    splitted_mode = PORT_MODE_NULL;
    forced_split  = false;
    moving_cursor = false;
    mouse_down    = false;

    // Set Font
    font_name = QFont(canvas.theme->box_font_name, canvas.theme->box_font_size, canvas.theme->box_font_state);
    font_port = QFont(canvas.theme->port_font_name, canvas.theme->port_font_size, canvas.theme->port_font_state);

    // Icon
    icon_svg = new CanvasIcon(icon, group_name, this);

    // Shadow
    if (options.fancy_eyecandy)
    {
        shadow = new CanvasBoxShadow(toGraphicsObject());
        shadow->setFakeParent(this);
        setGraphicsEffect(shadow);
    } else
        shadow = 0;

    // Final touches
    setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);

    // Wait for at least 1 port
    if (options.auto_hide_groups || options.fancy_eyecandy)
        setVisible(false);

    updatePositions();
}

CanvasBox::~CanvasBox()
{
    if (shadow)
        delete shadow;
    delete icon_svg;
}

int CanvasBox::getGroupId()
{
    return group_id;
}

QString CanvasBox::getGroupName()
{
    return group_name;
}

bool CanvasBox::isSplitted()
{
    return splitted;
}

PortMode CanvasBox::getSplittedMode()
{
    return splitted_mode;
}

int CanvasBox::getPortCount()
{
    return port_list_ids.count();
}

QList<int> CanvasBox::getPortList()
{
    return port_list_ids;
}

void CanvasBox::setIcon(Icon icon)
{
    icon_svg->setIcon(icon, group_name);
}

void CanvasBox::setSplit(bool split, PortMode mode)
{
    splitted = split;
    splitted_mode = mode;
}

void CanvasBox::setGroupName(QString group_name_)
{
    group_name = group_name_;
    updatePositions();
}

void CanvasBox::makeItGlow(int port_id, bool yesno)
{
    for (int i=0; i < canvas.connection_list.count(); i++)
    {
        if (canvas.connection_list[i].port_out_id == port_id || canvas.connection_list[i].port_in_id == port_id)
        {
            if (options.bezier_lines)
                ((CanvasBezierLine*)canvas.connection_list[i].widget)->setLineSelected(yesno);
            else
                ((CanvasLine*)canvas.connection_list[i].widget)->setLineSelected(yesno);
        }
    }
}

void CanvasBox::addLineFromGroup(QGraphicsItem* line, int connection_id)
{
    cb_line_t new_cbline = { line, connection_id };
    connection_lines.append(new_cbline);
}

void CanvasBox::removeLineFromGroup(int connection_id)
{
    for (int i=0; i < connection_lines.count(); i++)
    {
        if (connection_lines[i].connection_id == connection_id)
        {
            connection_lines.takeAt(i);
            return;
        }
    }

    qCritical("PatchCanvas::CanvasBox->removeLineFromGroup() - Unable to find line to remove");
}

CanvasPort* CanvasBox::addPortFromGroup(int port_id, QString port_name, PortMode port_mode, PortType port_type)
{
    if (port_list.count() == 0)
    {
        if (options.fancy_eyecandy)
            ItemFX(this, true);
        if (options.auto_hide_groups)
            setVisible(true);
    }

    CanvasPort* new_widget = new CanvasPort(port_id, port_name, port_mode, port_type, this);

    port_dict_t port_dict;
    port_dict.port_id   = port_id;
    port_dict.port_name = port_name;
    port_dict.port_mode = port_mode;
    port_dict.port_type = port_type;
    port_dict.widget    = new_widget;

    port_list.append(port_dict);
    port_list_ids.append(port_id);
    updatePositions();

    return new_widget;
}

void CanvasBox::removePortFromGroup(int port_id)
{
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_id == port_id)
        {
            delete port_list[i].widget;
            port_list.takeAt(i);
            port_list_ids.takeAt(i);
            break;
        }
    }

    if (port_list_ids.contains(port_id))
    {
        qCritical("PatchCanvas::CanvasBox->removePort()) - Unable to find port to remove");
        return;
    }

    updatePositions();

    if (port_list.count() == 0 && isVisible())
    {
        if (canvas.debug and options.auto_hide_groups)
            qDebug("PatchCanvas::CanvasBox->removePort() - This group has no more ports, hide it");
        if (options.fancy_eyecandy)
            ItemFX(this, false, false);
        else if (options.auto_hide_groups)
            setVisible(false);
    }
}

void CanvasBox::renamePortFromGroup(int port_id, QString new_port_name)
{
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_id == port_id)
        {
            port_list[i].port_name = new_port_name;
            break;
        }
    }

    updatePositions();
}

void CanvasBox::checkItemPos()
{
    if (!canvas.size_rect.isNull())
    {
        QPointF pos = scenePos();
        if (!canvas.size_rect.contains(pos))
        {
            if (pos.x() < canvas.size_rect.x())
                setPos(canvas.size_rect.x(), pos.y());
            else if (pos.y() < canvas.size_rect.y())
                setPos(pos.x(), canvas.size_rect.y());
            else if (pos.x() > canvas.size_rect.width())
                setPos(canvas.size_rect.width(), pos.y());
            else if (pos.y() > canvas.size_rect.height())
                setPos(pos.x(), canvas.size_rect.height());
        }
    }
}

void CanvasBox::removeIconFromScene()
{
    canvas.scene->removeItem(icon_svg);
}

void CanvasBox::updatePositions()
{
    prepareGeometryChange();

    int max_in_width   = 0;
    int max_in_height  = 24;
    int max_out_width  = 0;
    int max_out_height = 24;
    bool have_audio_jack_in, have_audio_jack_out, have_midi_jack_in, have_midi_jack_out;
    bool have_midi_a2j_in,  have_midi_a2j_out, have_midi_alsa_in,  have_midi_alsa_out;
    have_audio_jack_in = have_audio_jack_out = have_midi_jack_in = have_midi_jack_out = false;
    have_midi_a2j_in = have_midi_a2j_out = have_midi_alsa_in = have_midi_alsa_out = false;

    // reset box size
    box_width  = 50;
    box_height = 25;

    // Check Text Name size
    int app_name_size = QFontMetrics(font_name).width(group_name)+30;
    if (app_name_size > box_width)
        box_width = app_name_size;

    // Get Max Box Width/Height
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT)
        {
            max_in_height += 18;

            int size = QFontMetrics(font_port).width(port_list[i].port_name);
            if (size > max_in_width)
                max_in_width = size;

            if (port_list[i].port_type == PORT_TYPE_AUDIO_JACK && !have_audio_jack_in)
            {
                have_audio_jack_in = true;
                max_in_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI_JACK && !have_midi_jack_in)
            {
                have_midi_jack_in = true;
                max_in_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI_A2J && !have_midi_a2j_in)
            {
                have_midi_a2j_in = true;
                max_in_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI_ALSA && !have_midi_alsa_in)
            {
                have_midi_alsa_in = true;
                max_in_height += 2;
            }
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT)
        {
            max_out_height += 18;

            int size = QFontMetrics(font_port).width(port_list[i].port_name);
            if (size > max_out_width)
                max_out_width = size;

            if (port_list[i].port_type == PORT_TYPE_AUDIO_JACK && !have_audio_jack_out)
            {
                have_audio_jack_out = true;
                max_out_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI_JACK && !have_midi_jack_out)
            {
                have_midi_jack_out = true;
                max_out_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI_A2J && !have_midi_a2j_out)
            {
                have_midi_a2j_out = true;
                max_out_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI_ALSA && !have_midi_alsa_out)
            {
                have_midi_alsa_out = true;
                max_out_height += 2;
            }
        }
    }

    int final_width = 30 + max_in_width + max_out_width;
    if (final_width > box_width)
        box_width = final_width;

    if (max_in_height > box_height)
        box_height = max_in_height;

    if (max_out_height > box_height)
        box_height = max_out_height;

    // Remove bottom space
    box_height -= 2;

    int last_in_pos  = 24;
    int last_out_pos = 24;
    PortType last_in_type  = PORT_TYPE_NULL;
    PortType last_out_type = PORT_TYPE_NULL;

    // Re-position ports, AUDIO_JACK
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT && port_list[i].port_type == PORT_TYPE_AUDIO_JACK)
        {
            port_list[i].widget->setPos(QPointF(1, last_in_pos));
            port_list[i].widget->setPortWidth(max_in_width);

            last_in_pos += 18;
            last_in_type = port_list[i].port_type;
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT && port_list[i].port_type == PORT_TYPE_AUDIO_JACK)
        {
            port_list[i].widget->setPos(QPointF(box_width-max_out_width-13, last_out_pos));
            port_list[i].widget->setPortWidth(max_out_width);

            last_out_pos += 18;
            last_out_type = port_list[i].port_type;
        }
    }

    // Re-position ports, MIDI_JACK
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT && port_list[i].port_type == PORT_TYPE_MIDI_JACK)
        {
            if (last_in_type != PORT_TYPE_NULL && port_list[i].port_type != last_in_type)
                last_in_pos += 2;

            port_list[i].widget->setPos(QPointF(1, last_in_pos));
            port_list[i].widget->setPortWidth(max_in_width);

            last_in_pos += 18;
            last_in_type = port_list[i].port_type;
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT && port_list[i].port_type == PORT_TYPE_MIDI_JACK)
        {
            if (last_out_type != PORT_TYPE_NULL && port_list[i].port_type != last_out_type)
                last_out_pos += 2;

            port_list[i].widget->setPos(QPointF(box_width-max_out_width-13, last_out_pos));
            port_list[i].widget->setPortWidth(max_out_width);

            last_out_pos += 18;
            last_out_type = port_list[i].port_type;
        }
    }

    // Re-position ports, MIDI_A2J
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT && port_list[i].port_type == PORT_TYPE_MIDI_A2J)
        {
            if (last_in_type != PORT_TYPE_NULL && port_list[i].port_type != last_in_type)
                last_in_pos += 2;

            port_list[i].widget->setPos(QPointF(1, last_in_pos));
            port_list[i].widget->setPortWidth(max_in_width);

            last_in_pos += 18;
            last_in_type = port_list[i].port_type;
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT && port_list[i].port_type == PORT_TYPE_MIDI_A2J)
        {
            if (last_out_type != PORT_TYPE_NULL && port_list[i].port_type != last_out_type)
                last_out_pos += 2;

            port_list[i].widget->setPos(QPointF(box_width-max_out_width-13, last_out_pos));
            port_list[i].widget->setPortWidth(max_out_width);

            last_out_pos += 18;
            last_out_type = port_list[i].port_type;
        }
    }

    // Re-position ports, MIDI_ALSA
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT && port_list[i].port_type == PORT_TYPE_MIDI_ALSA)
        {
            if (last_in_type != PORT_TYPE_NULL && port_list[i].port_type != last_in_type)
                last_in_pos += 2;

            port_list[i].widget->setPos(QPointF(1, last_in_pos));
            port_list[i].widget->setPortWidth(max_in_width);

            last_in_pos += 18;
            last_in_type = port_list[i].port_type;
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT && port_list[i].port_type == PORT_TYPE_MIDI_ALSA)
        {
            if (last_out_type != PORT_TYPE_NULL && port_list[i].port_type != last_out_type)
                last_out_pos += 2;

            port_list[i].widget->setPos(QPointF(box_width-max_out_width-13, last_out_pos));
            port_list[i].widget->setPortWidth(max_out_width);

            last_out_pos += 18;
            last_out_type = port_list[i].port_type;
        }
    }

    for (int i=0; i < connection_lines.count(); i++)
    {
        //QTimer::singleShot(0, connection_lines[i].line->toGraphicsObject(), SIGNAL(updateLinePos()));
    }

    update();
}

void CanvasBox::resetLinesZValue()
{
    for (int i=0; i < connection_lines.count(); i++)
    {
        int z_value;
        if (port_list_ids.contains(canvas.connection_list[i].port_out_id) && port_list_ids.contains(canvas.connection_list[i].port_in_id))
            z_value = canvas.last_z_value;
        else
            z_value = canvas.last_z_value-1;

        canvas.connection_list[i].widget->setZValue(z_value);
    }
}

void CanvasBox::repaintLines()
{
    if (pos() != last_pos)
    {
      for (int i=0; i < connection_lines.count(); i++)
      {
          if (options.bezier_lines)
              ((CanvasBezierLine*)connection_lines[i].line)->updateLinePos();
          else
              ((CanvasLine*)connection_lines[i].line)->updateLinePos();
      }
    }

    last_pos = pos();
}

int CanvasBox::type() const
{
    return CanvasBoxType;
}

//contextMenuEvent(self, event)
//contextMenuDisconnect(self, port_id)

void CanvasBox::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    canvas.last_z_value += 1;
    setZValue(canvas.last_z_value);
    resetLinesZValue();
    moving_cursor = false;

    if (event->button() == Qt::RightButton)
    {
        canvas.scene->clearSelection();
        setSelected(true);
        mouse_down = false;
        event->accept();
        return;
    }
    else if (event->button() == Qt::LeftButton)
    {
        if (sceneBoundingRect().contains(event->scenePos()))
            mouse_down = true;
        else
        {
            // Fixes a weird Qt behaviour with right-click mouseMove
            mouse_down = false;
            event->ignore();
            return;
        }
    }
    else
        mouse_down = false;

    QGraphicsItem::mousePressEvent(event);
}

void CanvasBox::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (mouse_down)
    {
        if (!moving_cursor)
        {
            setCursor(QCursor(Qt::SizeAllCursor));
            moving_cursor = true;
        }
        repaintLines();
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void CanvasBox::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (moving_cursor)
        setCursor(QCursor(Qt::ArrowCursor));
    mouse_down = false;
    moving_cursor = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

QRectF CanvasBox::boundingRect() const
{
    return QRectF(0, 0, box_width, box_height);
}

void CanvasBox::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, false);

    if (isSelected())
        painter->setPen(canvas.theme->box_pen_sel);
    else
        painter->setPen(canvas.theme->box_pen);

    QLinearGradient box_gradient(0, 0, 0, box_height);
    box_gradient.setColorAt(0, canvas.theme->box_bg_1);
    box_gradient.setColorAt(1, canvas.theme->box_bg_2);

    painter->setBrush(box_gradient);
    painter->drawRect(0, 0, box_width, box_height);

    QPointF text_pos(25, 16);

    painter->setFont(font_name);
    painter->setPen(canvas.theme->box_text);
    painter->drawText(text_pos, group_name);

    Q_UNUSED(option);
    Q_UNUSED(widget);
}

END_NAMESPACE_PATCHCANVAS
