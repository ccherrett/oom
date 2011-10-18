#include "canvasbox.h"

#include <QCursor>

CanvasBox::CanvasBox(int group_id_, QString text_, PatchCanvas::Icon icon, PatchScene* scene, PatchCanvas::Canvas* canvas_) :
    QGraphicsItem(0, scene),
    canvas(canvas_)
{
    // Save Variables, useful for later
    group_id = group_id_;
    text = text_;

    // Base Variables
    box_width  = 50;
    box_height = 25;
//    port_list = [];
//    port_list_ids = [];
//    connection_lines = [];

//    last_pos = None;
    splitted = false;
    splitted_mode = PatchCanvas::PORT_MODE_NULL;
    forced_split = false;
    moving_cursor = false;
    mouse_down = false;

    // Set Font
    font_name = QFont(canvas->theme->box_font_name, canvas->theme->box_font_size, canvas->theme->box_font_state);
    font_port = QFont(canvas->theme->port_font_name, canvas->theme->port_font_size, canvas->theme->port_font_state);

    // Icon
    icon_svg = new CanvasIcon(icon, text, this, canvas);

    // ...

    // Final touches
    setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);

    // Initial Paint
    //if (options['auto_hide_groups'] or options['fancy_eyecandy']):
    //setVisible(false); // Wait for at least 1 port

    relocateAll();
}

CanvasBox::~CanvasBox()
{
    delete icon_svg;
}

void CanvasBox::setIcon(PatchCanvas::Icon icon)
{
    icon_svg->setIcon(icon, text);
}

void CanvasBox::setSplit(bool split, PatchCanvas::PortMode mode)
{
    splitted = split;
    splitted_mode = mode;
}

void CanvasBox::setText(QString text_)
{
    text = text_;
    relocateAll();
}

//makeItGlow(self, port_id, glow)
//addLine(self, line, connection_id)
//removeLine(self, connection_id)
//addPort(self, port_id, port_name, port_mode, port_type)
//removePort(self, port_id)
//renamePort(self, port_id, new_port_name)

void CanvasBox::relocateAll()
{
    prepareGeometryChange();

    //int max_in_width  = 0;
    //int max_in_height = 24;
    //int max_out_width = 0;
    //int max_out_height = 24;
    //bool have_audio_in, have_audio_out, have_midi_in, have_midi_out, have_outro_in,  have_outro_out;
    //have_audio_in = have_audio_out = have_midi_in = have_midi_out = have_outro_in = have_outro_out = false;

    // reset box size
    box_width  = 50;
    box_height = 25;

    // Check Text Name size
    int app_name_size = QFontMetrics(font_name).width(text)+30;
    if (app_name_size > box_width)
        box_width = app_name_size;

    // TODO

    update();
}

void CanvasBox::resetLinesZValue()
{

}

void CanvasBox::repaintLines()
{

}

//checkItemPos(self)
//contextMenuEvent(self, event)
//contextMenuDisconnect(self, port_id)

void CanvasBox::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    canvas->last_z_value += 1;
    setZValue(canvas->last_z_value);
    resetLinesZValue();
    moving_cursor = false;

    if (event->button() == Qt::RightButton)
    {
      canvas->scene->clearSelection();
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
      painter->setPen(canvas->theme->box_pen_sel);
    else
      painter->setPen(canvas->theme->box_pen);

    QLinearGradient box_gradient(0, 0, 0, box_height);
    box_gradient.setColorAt(0, canvas->theme->box_bg_1);
    box_gradient.setColorAt(1, canvas->theme->box_bg_2);

    painter->setBrush(box_gradient);
    painter->drawRect(0, 0, box_width, box_height);

    QPointF text_pos(25, 16);

    painter->setFont(font_name);
    painter->setPen(canvas->theme->box_text);
    painter->drawText(text_pos, text);

    Q_UNUSED(option);
    Q_UNUSED(widget);
}
