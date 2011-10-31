#ifndef CANVASLINE_H
#define CANVASLINE_H

#include "patchcanvas.h"

#include <QGraphicsLineItem>

class QPainter;

START_NAMESPACE_PATCHCANVAS

class CanvasPort;
class CanvasPortGlow;

class CanvasLine : public QGraphicsLineItem
{
public:
    CanvasLine(CanvasPort* item1, CanvasPort* item2, QGraphicsItem* parent);
    ~CanvasLine();

    bool isLocked();
    void setLocked(bool yesno);

    void enableGlow(bool yesno);

    void updateLinePos();
    void updateLineGradient(bool selected=false);

    int type() const;

private:
    CanvasPort* item1;
    CanvasPort* item2;
    PortType port_type1;
    PortType port_type2;
    CanvasPortGlow* glow;
    bool locked;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASLINE_H
