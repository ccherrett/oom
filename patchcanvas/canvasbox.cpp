#include "canvasbox.h"

CanvasBox::CanvasBox(int group_id_, QString text_, PatchCanvas::Icon icon_, Theme* theme_, QGraphicsScene* scene) :
    QGraphicsItem(0, scene),
    group_id(group_id_),
    text(text_),
    icon(icon_),
    theme(theme_)
{
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
    repositioned = false;
    moving_cursor = false;

    // Set Font
    font_name = QFont(theme->box_font_name, theme->box_font_size, theme->box_font_state);
    font_port = QFont(theme->port_font_name, theme->port_font_size, theme->port_font_state);

    // ...

    // Final touches
    setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);

    // Initial Paint
    //if (options['auto_hide_groups'] or options['fancy_eyecandy']):
    //setVisible(false); // Wait for at least 1 port

    relocateAll();
}

void CanvasBox::setIcon(PatchCanvas::Icon icon)
{
    //icon_svg->setIcon(icon, text);
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

void CanvasBox::addLine(void* line, int connection_id)
{
    //connection_lines.append([line, connection_id]);
}

void CanvasBox::relocateAll()
{

}

QRectF CanvasBox::boundingRect() const
{
    return QRectF(0, 0, box_width, box_height);
}

void CanvasBox::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, false);

    //repaintLines();

    if (isSelected())
      painter->setPen(theme->box_pen_sel);
    else
      painter->setPen(theme->box_pen);

    QLinearGradient box_gradient(0, 0, 0, box_height);
    box_gradient.setColorAt(0, theme->box_bg_1);
    box_gradient.setColorAt(1, theme->box_bg_2);

    painter->setBrush(box_gradient);
    painter->drawRect(0, 0, box_width, box_height);

    QPointF text_pos(25, 16);

    painter->setFont(font_name);
    painter->setPen(theme->box_text);
    painter->drawText(text_pos, text);

    Q_UNUSED(option);
    Q_UNUSED(widget);
}
