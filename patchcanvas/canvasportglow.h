#ifndef CANVASPORTGLOW_H
#define CANVASPORTGLOW_H

#include "patchcanvas.h"

#include <QGraphicsDropShadowEffect>

START_NAMESPACE_PATCHCANVAS

class CanvasPortGlow : public QGraphicsDropShadowEffect
{
public:
    CanvasPortGlow(PortType port_type, QObject* parent);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASPORTGLOW_H
