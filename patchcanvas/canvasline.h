#ifndef CANVASLINE_H
#define CANVASLINE_H

#include "patchcanvas.h"
#include <QGraphicsLineItem>
#include <QPainter>

START_NAMESPACE_PATCHCANVAS

class CanvasPort;
class CanvasPortGlow;

class CanvasLine : public QGraphicsLineItem
{
public:
    CanvasLine(CanvasPort* item1, CanvasPort* item2, QGraphicsItem* parent);
    ~CanvasLine();

    void enableGlow(bool yesno);
    void setPortType(PortType port_type1, PortType port_type2);

    void setLocked(bool yesno);
    bool isLocked();

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
