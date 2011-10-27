#ifndef CANVASBOXSHADOW_H
#define CANVASBOXSHADOW_H

#include "patchcanvas.h"
#include <QGraphicsDropShadowEffect>

START_NAMESPACE_PATCHCANVAS

class CanvasBox;

class CanvasBoxShadow : public QGraphicsDropShadowEffect
{
public:
    CanvasBoxShadow(QObject* parent);
    void setFakeParent(CanvasBox* fake_parent);

protected:
    void draw(QPainter* painter);

private:
    CanvasBox* fake_parent;
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASBOXSHADOW_H
