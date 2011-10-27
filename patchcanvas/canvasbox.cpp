#include "canvasbox.h"
#include "canvasboxshadow.h"
#include "canvasicon.h"
#include "canvasline.h"
#include "canvasbezierline.h"
#include "canvasport.h"

#include <cstdio>
#include <QCursor>
#include <QFontMetrics>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;
extern options_t options;

CanvasBox::CanvasBox(int group_id_, QString text_, Icon icon, QGraphicsItem* parent) :
    QGraphicsItem(parent, canvas.scene)
{
    // Save Variables, useful for later
    group_id = group_id_;
    text = text_;

    // Base Variables
    box_width  = 50;
    box_height = 25;
    port_list.clear();
    port_list_ids.clear();
    connection_lines.clear();

    last_pos = QPointF();
    splitted = false;
    splitted_mode = PORT_MODE_NULL;
    forced_split = false;
    moving_cursor = false;
    mouse_down = false;

    // Set Font
    font_name = QFont(canvas.theme->box_font_name, canvas.theme->box_font_size, canvas.theme->box_font_state);
    font_port = QFont(canvas.theme->port_font_name, canvas.theme->port_font_size, canvas.theme->port_font_state);

    // Icon
    icon_svg = new CanvasIcon(icon, text, this);

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

    // Initial Paint
    //if (options.auto_hide_groups || options.fancy_eyecandy)
    //    setVisible(false); // Wait for at least 1 port

    relocateAll();
}

CanvasBox::~CanvasBox()
{
    if (shadow)
        delete shadow;
    delete icon_svg;
}

QString CanvasBox::getText()
{
    return text;
}

bool CanvasBox::isSplitted()
{
    return splitted;
}

PortMode CanvasBox::getSplittedMode()
{
    return splitted_mode;
}

QList<port_dict_t> CanvasBox::getPortList()
{
    return port_list;
}

int CanvasBox::getPortCount()
{
    return port_list.count();
}

void CanvasBox::setIcon(Icon icon)
{
    icon_svg->setIcon(icon, text);
}

void CanvasBox::setSplit(bool split, PortMode mode)
{
    splitted = split;
    splitted_mode = mode;
}

void CanvasBox::setText(QString text_)
{
    text = text_;
    relocateAll();
}

void CanvasBox::makeItGlow(int port_id, bool yesno)
{
    for (int i=0; i < canvas.connection_list.count(); i++)
    {
        if (canvas.connection_list[i].port_out_id == port_id || canvas.connection_list[i].port_in_id == port_id)
        {
            if (options.bezier_lines)
                ((CanvasBezierLine*)canvas.connection_list[i].widget)->enableGlow(yesno);
            else
                ((CanvasLine*)canvas.connection_list[i].widget)->enableGlow(yesno);
        }
    }
}

void CanvasBox::addLine(QGraphicsItem* line, int connection_id)
{
    cb_line_t new_cbline = { line, connection_id };
    connection_lines.append(new_cbline);
}

void CanvasBox::removeLine(int connection_id)
{
    for (int i=0; i < connection_lines.count(); i++)
    {
        if (connection_lines[i].connection_id == connection_id)
        {
            connection_lines.takeAt(i);
            return;
        }
    }

    printf("PatchCanvas::CanvasBox->removeLine() - Unable to find line to remove\n");
}

port_dict_t CanvasBox::addPort(int port_id, QString port_name, PortMode port_mode, PortType port_type)
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
    relocateAll();

    return port_dict;
}

void CanvasBox::removePort(int port_id)
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
        printf("PatchCanvas::CanvasBox->removePort()) - Unable to find port to remove\n");
        return;
    }

    relocateAll();

    if (port_list.count() == 0 && isVisible())
    {
        if (canvas.debug and options.auto_hide_groups)
            printf("PatchCanvas::CanvasBox->removePort() - This group has no more ports, hide it\n");
        if (options.fancy_eyecandy)
            ItemFX(this, false, false);
        else if (options.auto_hide_groups)
            setVisible(false);
    }
}

void CanvasBox::renamePort(int port_id, QString new_port_name)
{
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_id == port_id)
        {
            port_list[i].port_name = new_port_name;
            break;
        }
    }

    relocateAll();
}

void CanvasBox::removeIconFromScene()
{
    canvas.scene->removeItem(icon_svg);
}

void CanvasBox::relocateAll()
{
    prepareGeometryChange();

    int max_in_width   = 0;
    int max_in_height  = 24;
    int max_out_width  = 0;
    int max_out_height = 24;
    bool have_audio_in, have_audio_out, have_midi_in, have_midi_out, have_outro_in,  have_outro_out;
    have_audio_in = have_audio_out = have_midi_in = have_midi_out = have_outro_in = have_outro_out = false;

    // reset box size
    box_width  = 50;
    box_height = 25;

    // Check Text Name size
    int app_name_size = QFontMetrics(font_name).width(text)+30;
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

            if (port_list[i].port_type == PORT_TYPE_AUDIO && !have_audio_in)
            {
                have_audio_in = true;
                max_in_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI && !have_midi_in)
            {
                have_midi_in = true;
                max_in_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_OUTRO && !have_outro_in)
            {
                have_outro_in = true;
                max_in_height += 2;
            }
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT)
        {
            max_out_height += 18;

            int size = QFontMetrics(font_port).width(port_list[i].port_name);
            if (size > max_out_width)
                max_out_width = size;

            if (port_list[i].port_type == PORT_TYPE_AUDIO && !have_audio_out)
            {
                have_audio_out = true;
                max_out_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_MIDI && !have_midi_out)
            {
                have_midi_out = true;
                max_out_height += 2;
            }
            else if (port_list[i].port_type == PORT_TYPE_OUTRO && !have_outro_out)
            {
                have_outro_out = true;
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

    // Re-position ports, AUDIO
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT && port_list[i].port_type == PORT_TYPE_AUDIO)
        {
            port_list[i].widget->setPos(QPointF(1, last_in_pos));
            port_list[i].widget->setPortWidth(max_in_width);

            last_in_pos += 18;
            last_in_type = port_list[i].port_type;
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT && port_list[i].port_type == PORT_TYPE_AUDIO)
        {
            port_list[i].widget->setPos(QPointF(box_width-max_out_width-13, last_out_pos));
            port_list[i].widget->setPortWidth(max_out_width);

            last_out_pos += 18;
            last_out_type = port_list[i].port_type;
        }
    }

    // Re-position ports, MIDI
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT && port_list[i].port_type == PORT_TYPE_MIDI)
        {
            if (last_in_type != PORT_TYPE_NULL && port_list[i].port_type != last_in_type)
                last_in_pos += 2;

            port_list[i].widget->setPos(QPointF(1, last_in_pos));
            port_list[i].widget->setPortWidth(max_in_width);

            last_in_pos += 18;
            last_in_type = port_list[i].port_type;
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT && port_list[i].port_type == PORT_TYPE_MIDI)
        {
            if (last_out_type != PORT_TYPE_NULL && port_list[i].port_type != last_out_type)
                last_out_pos += 2;

            port_list[i].widget->setPos(QPointF(box_width-max_out_width-13, last_out_pos));
            port_list[i].widget->setPortWidth(max_out_width);

            last_out_pos += 18;
            last_out_type = port_list[i].port_type;
        }
    }

    // Re-position ports, Outro
    for (int i=0; i < port_list.count(); i++)
    {
        if (port_list[i].port_mode == PORT_MODE_INPUT && port_list[i].port_type == PORT_TYPE_OUTRO)
        {
            if (last_in_type != PORT_TYPE_NULL && port_list[i].port_type != last_in_type)
                last_in_pos += 2;

            port_list[i].widget->setPos(QPointF(1, last_in_pos));
            port_list[i].widget->setPortWidth(max_in_width);

            last_in_pos += 18;
            last_in_type = port_list[i].port_type;
        }
        else if (port_list[i].port_mode == PORT_MODE_OUTPUT && port_list[i].port_type == PORT_TYPE_OUTRO)
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
    }
    else if (event->button() == Qt::LeftButton) // FIXME - there's a bug laying around here, this code fixes it:
    {
        if (sceneBoundingRect().contains(event->scenePos()))
        {
            mouse_down = true;
            QGraphicsItem::mousePressEvent(event);
        }
        else
        {
            mouse_down = false;
            event->ignore();
        }
    }
    else
    {
        mouse_down = false;
        QGraphicsItem::mousePressEvent(event);
    }
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
        QGraphicsItem::mouseMoveEvent(event);
    }
    else
        event->accept();
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

    repaintLines();

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
    painter->drawText(text_pos, text);

    Q_UNUSED(option);
    Q_UNUSED(widget);
}

END_NAMESPACE_PATCHCANVAS
