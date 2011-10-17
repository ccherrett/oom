#ifndef PATCHSCENE_H
#define PATCHSCENE_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>

#include "theme.h"

class PatchScene : public QGraphicsScene
{
    Q_OBJECT

public:
    PatchScene();
    void rubberbandByTheme(Theme* theme);

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

#endif // PATCHSCENE_H
