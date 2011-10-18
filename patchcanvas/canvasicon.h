#ifndef CANVASICON_H
#define CANVASICON_H

#include <QGraphicsSvgItem>
#include <QGraphicsColorizeEffect>
#include <QPainter>
#include <QSvgRenderer>

#include "patchcanvas.h"

START_NAMESPACE_PATCHCANVAS

class CanvasIcon : public QGraphicsSvgItem
{
public:
    CanvasIcon(Icon icon, QString name, QGraphicsItem* parent);
    ~CanvasIcon();

    void setIcon(Icon icon, QString name);

private:
    QGraphicsColorizeEffect* colorFX;
    QSvgRenderer* renderer;
    QRectF size;

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASICON_H
