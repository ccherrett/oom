#ifndef PATCHSCENE_H
#define PATCHSCENE_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>

#include "patchcanvas.h"

START_NAMESPACE_PATCHCANVAS

class PatchScene : public QGraphicsScene
{
public:
    PatchScene();
    void rubberbandByTheme();

private:
    bool ctrl_down;
    bool mouse_down;
    bool fake_selection;
    QGraphicsRectItem* fake_rubberband;
    QPointF orig_point;

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    void wheelEvent(QGraphicsSceneWheelEvent* event);
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
};

END_NAMESPACE_PATCHCANVAS

#endif // PATCHSCENE_H
