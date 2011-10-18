#include "canvasboxshadow.h"

CanvasBoxShadow::CanvasBoxShadow(Theme* theme)
{
    setBlurRadius(20);
    setColor(theme->box_shadow);
    setOffset(0, 0);
    fake_parent = 0;
}

void CanvasBoxShadow::setFakeParent(CanvasBox* parent)
{
    fake_parent = parent;
}

void CanvasBoxShadow::draw(QPainter* painter)
{
    if (fake_parent)
        fake_parent->repaintLines();
    return QGraphicsDropShadowEffect::draw(painter);
}
