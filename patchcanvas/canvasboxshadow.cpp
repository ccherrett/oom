#include "canvasboxshadow.h"
#include "canvasbox.h"

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;

CanvasBoxShadow::CanvasBoxShadow(QObject* parent) :
    QGraphicsDropShadowEffect(parent)
{
    setBlurRadius(20);
    setColor(canvas.theme->box_shadow);
    setOffset(0, 0);
    fake_parent = 0;
}

void CanvasBoxShadow::setFakeParent(CanvasBox* fake_parent_)
{
    fake_parent = fake_parent_;
}

void CanvasBoxShadow::draw(QPainter* painter)
{
    if (fake_parent)
        fake_parent->repaintLines();
    return QGraphicsDropShadowEffect::draw(painter);
}

END_NAMESPACE_PATCHCANVAS
