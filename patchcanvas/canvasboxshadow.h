#ifndef CANVASBOXSHADOW_H
#define CANVASBOXSHADOW_H

#include <QGraphicsDropShadowEffect>

#include "patchcanvas.h"
#include "canvasbox.h"

class CanvasBoxShadow : public QGraphicsDropShadowEffect
{
public:
    CanvasBoxShadow(Theme* theme);
    void setFakeParent(CanvasBox* parent);
    void draw(QPainter* painter);

private:
    CanvasBox* fake_parent;
};

#endif // CANVASBOXSHADOW_H
