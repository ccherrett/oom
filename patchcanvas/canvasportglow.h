#ifndef CANVASPORTGLOW_H
#define CANVASPORTGLOW_H

#include <QGraphicsDropShadowEffect>

#include "patchcanvas.h"

class CanvasPortGlow : public QGraphicsDropShadowEffect
{
public:
    CanvasPortGlow();
    void setPortType(PatchCanvas::PortType port_type, Theme* theme);
};

#endif // CANVASPORTGLOW_H
