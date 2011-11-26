#ifndef CANVASBEZIERLINE_H
#define CANVASBEZIERLINE_H

#include "patchcanvas.h"

#include <QGraphicsPathItem>

class QPainter;

START_NAMESPACE_PATCHCANVAS

class CanvasPort;
class CanvasPortGlow;

class CanvasBezierLine : public QGraphicsPathItem
{
public:
    CanvasBezierLine(CanvasPort* item1, CanvasPort* item2, QGraphicsItem* parent);
    ~CanvasBezierLine();

    bool isLocked();
    void setLocked(bool yesno);

    bool isLineSelected();
    void setLineSelected(bool yesno);

    void updateLinePos();
    void updateLineGradient(bool selected);

    int type() const;

private:
    CanvasPort* item1;
    CanvasPort* item2;
    CanvasPortGlow* glow;
    bool locked;
    bool line_selected;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASBEZIERLINE_H
