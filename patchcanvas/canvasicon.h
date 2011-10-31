#ifndef CANVASICON_H
#define CANVASICON_H

#include "patchcanvas.h"

#include <QGraphicsSvgItem>

class QPainter;
class QGraphicsColorizeEffect;
class QSvgRenderer;

START_NAMESPACE_PATCHCANVAS

class CanvasIcon : public QGraphicsSvgItem
{
public:
    CanvasIcon(Icon icon, QString name, QGraphicsItem* parent);
    ~CanvasIcon();

    void setIcon(Icon icon, QString name);

    int type() const;

private:
    QGraphicsColorizeEffect* colorFX;
    QSvgRenderer* renderer;
    QRectF size;

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASICON_H
