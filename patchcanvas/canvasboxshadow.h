#ifndef CANVASBOXSHADOW_H
#define CANVASBOXSHADOW_H

#include <QGraphicsDropShadowEffect>

#include "patchcanvas.h"
#include "canvasbox.h"

START_NAMESPACE_PATCHCANVAS

class CanvasBoxShadow : public QGraphicsDropShadowEffect
{
public:
    CanvasBoxShadow(QObject* parent);
    void setFakeParent(CanvasBox* fake_parent);
    void draw(QPainter* painter);

private:
    CanvasBox* fake_parent;
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASBOXSHADOW_H
