#ifndef CANVASICON_H
#define CANVASICON_H

#include <QGraphicsSvgItem>
#include <QGraphicsColorizeEffect>
#include <QPainter>
#include <QSvgRenderer>

#include "patchcanvas.h"

class CanvasIcon : public QGraphicsSvgItem
{
public:
    CanvasIcon(PatchCanvas::Icon icon, QString name, QGraphicsItem* parent, PatchCanvas::Canvas* canvas);
    ~CanvasIcon();

    void setIcon(PatchCanvas::Icon icon, QString name);

private:
    PatchCanvas::Canvas* canvas;

    QGraphicsColorizeEffect* colorFX;
    QSvgRenderer* renderer;
    QRectF size;

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

#endif // CANVASICON_H
