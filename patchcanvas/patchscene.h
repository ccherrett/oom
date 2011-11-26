#ifndef PATCHSCENE_H
#define PATCHSCENE_H

#include <QGraphicsScene>

#include "patchcanvas-api.h"

class QKeyEvent;
class QGraphicsRectItem;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;

START_NAMESPACE_PATCHCANVAS

class PatchScene : public QGraphicsScene
{
public:
    PatchScene(QObject* parent = 0);
    void rubberbandByTheme();
    void fixScaleFactor();
    void zoom_fit();
    void zoom_in();
    void zoom_out();
    void zoom_reset();

private:
    bool ctrl_down;
    bool mouse_down_init;
    bool mouse_rubberband;
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
