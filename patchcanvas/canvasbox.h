#ifndef CANVASBOX_H
#define CANVASBOX_H

#include <QGraphicsItem>
#include <QFont>
#include <QPainter>

#include "patchcanvas.h"

class CanvasBox : public QGraphicsItem
{
public:
    CanvasBox(int group_id, QString text, PatchCanvas::Icon icon, Theme* theme, QGraphicsScene* scene = 0);

    void setIcon(PatchCanvas::Icon icon);
    void setSplit(bool split, PatchCanvas::PortMode mode);
    void setText(QString text);
    void addLine(void* line, int connection_id);

    void relocateAll();

private:
    int group_id;
    QString text;
    PatchCanvas::Icon icon;
    Theme* theme;

    int box_width;
    int box_height;

    bool splitted;
    PatchCanvas::PortMode splitted_mode;
    bool forced_split;
    bool repositioned;
    bool moving_cursor;

    QFont font_name;
    QFont font_port;


    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

#endif // CANVASBOX_H
