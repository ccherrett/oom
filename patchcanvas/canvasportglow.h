#ifndef CANVASPORTGLOW_H
#define CANVASPORTGLOW_H

#include "patchcanvas.h"
#include <QGraphicsDropShadowEffect>

START_NAMESPACE_PATCHCANVAS

class CanvasPortGlow : public QGraphicsDropShadowEffect
{
public:
    CanvasPortGlow(QObject* parent);
    void setPortType(PortType port_type);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASPORTGLOW_H
