#include "patchscene.h"
#include "patchcanvas.h"
#include "canvasbox.h"

#include <cmath>

#include <QKeyEvent>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;

PatchScene::PatchScene(QObject* parent) :
        QGraphicsScene(parent)
{
    ctrl_down  = false;
    mouse_down = false;
    fake_selection  = false;
    fake_rubberband = addRect(QRectF(0, 0, 0, 0));
    fake_rubberband->setZValue(-1);
    fake_rubberband->hide();
    orig_point = QPointF(0, 0);
}

void PatchScene::rubberbandByTheme()
{
    fake_rubberband->setPen(canvas.theme->rubberband_pen);
    fake_rubberband->setBrush(canvas.theme->rubberband_brush);
}

void PatchScene::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control)
        ctrl_down = true;
    QGraphicsScene::keyPressEvent(event);
}

void PatchScene::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control)
        ctrl_down = false;
    QGraphicsScene::keyReleaseEvent(event);
}

void PatchScene::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    if (ctrl_down)
    {
        double factor = pow(1.41, (event->delta()/240.0));
        QGraphicsView* const view = views().at(0);
        view->scale(factor, factor);
    }
    QGraphicsScene::wheelEvent(event);
}

void PatchScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
//    if (event->button() == Qt::LeftButton)
//        mouse_down = true;
//    else
//        mouse_down = false;
    QGraphicsScene::mousePressEvent(event);
}

void PatchScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (mouse_down)
    {
        if (!fake_selection)
        {
            orig_point = event->scenePos();
            fake_rubberband->show();
            fake_selection = true;
        }

        QPointF pos = event->scenePos();
        int x, y;

        if (pos.x() > orig_point.x()) x = orig_point.x();
        else x = pos.x();

        if (pos.y() > orig_point.y()) y = orig_point.y();
        else y = pos.y();

        fake_rubberband->setRect(x, y, abs(pos.x()-orig_point.x()), abs(pos.y()-orig_point.y()));
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void PatchScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (fake_selection)
    {
        QList<QGraphicsItem*> items_list = items();
        if (items_list.count() > 0)
        {
            for (int i=0; i < items_list.count(); i++)
            {
                if (items_list[i]->isVisible() && items_list[i]->type() == CanvasBoxType)
                {
                    QRectF item_rect = items_list[i]->sceneBoundingRect();
                    if ( fake_rubberband->contains(QPointF(item_rect.x(), item_rect.y())) &&
                         fake_rubberband->contains(QPointF(item_rect.x()+item_rect.width(), item_rect.y()+item_rect.height())) )
                        items_list[i]->setSelected(true);
                }
            }

            fake_rubberband->hide();
            fake_rubberband->setRect(0, 0, 0, 0);
            fake_selection = false;
        }
    }
    else
    {
        QList<QGraphicsItem*> items_list = selectedItems();

        for (int i=0; i < items_list.count(); i++)
        {
            if (items_list[i]->isVisible() && items_list[i]->type() == CanvasBoxType)
            {
                CanvasBox* cbox = (CanvasBox*)items_list[i];
                cbox->checkItemPos();
                //emit(SIGNAL(sceneGroupMoved(int, PortMode, QPointF)), cbox->getGroupId(), cbox->getSplittedMode(), cbox->scenePos());
            }
        }
    }

    mouse_down = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

END_NAMESPACE_PATCHCANVAS
