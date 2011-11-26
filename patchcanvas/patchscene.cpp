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
    mouse_down_init  = false;
    mouse_rubberband = false;
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

void PatchScene::fixScaleFactor()
{
    QGraphicsView* const view = views().at(0);
    qreal scale = view->transform().m11();
    if (scale > 3.0)
    {
      view->resetTransform();
      view->scale(3.0, 3.0);
    }
    else if (scale < 0.2)
    {
      view->resetTransform();
      view->scale(0.2, 0.2);
    }
}

void PatchScene::zoom_fit()
{
#define qreal_null -0xfffe
    qreal min_x, min_y, max_x, max_y;
    min_x = min_y = max_x = max_y = qreal_null;
    QList<QGraphicsItem*> items_list = items();

    if (items_list.count() > 0)
    {
        for (int i=0; i < items_list.count(); i++)
        {
            if (items_list[i]->isVisible() and items_list[i]->type() == CanvasBoxType)
            {
                QPointF pos = items_list[i]->scenePos();
                QRectF rect = items_list[i]->boundingRect();

                if (min_x == qreal_null)
                    min_x = pos.x();
                else if (pos.x() < min_x)
                    min_x = pos.x();

                if (min_y == qreal_null)
                    min_y = pos.y();
                else if (pos.y() < min_y)
                    min_y = pos.y();

                if (max_x == qreal_null)
                    max_x = pos.x()+rect.width();
                else if (pos.x()+rect.width() > max_x)
                    max_x = pos.x()+rect.width();

                if (max_y == qreal_null)
                    max_y = pos.y()+rect.height();
                else if (pos.y()+rect.height() > max_y)
                    max_y = pos.y()+rect.height();
            }
        }

        views().at(0)->fitInView(min_x, min_y, abs(max_x-min_x), abs(max_y-min_y), Qt::KeepAspectRatio);
        fixScaleFactor();
    }
}

void PatchScene::zoom_in()
{
    QGraphicsView* const view = views().at(0);
    if (view->transform().m11() < 3.0)
        view->scale(1.2, 1.2);
}

void PatchScene::zoom_out()
{
    QGraphicsView* const view = views().at(0);
    if (view->transform().m11() > 0.2)
        view->scale(0.8, 0.8);
}
void PatchScene::zoom_reset()
{
    QGraphicsView* const view = views().at(0);
    view->resetTransform();
}
void PatchScene::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control)
        ctrl_down = true;

    if (event->key() == Qt::Key_Home)
    {
        zoom_fit();
        event->accept();
    }

    else if (ctrl_down)
    {
        if (event->key() == Qt::Key_Plus)
        {
            zoom_in();
            event->accept();
        }
        else if (event->key() == Qt::Key_Minus)
        {
            zoom_out();
            event->accept();
        }
        else if (event->key() == Qt::Key_1)
        {
            zoom_reset();
            event->accept();
        }
        else
            QGraphicsScene::keyPressEvent(event);
    }
    else
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
        QGraphicsView* const view = views().at(0);
        double factor = pow(1.41, (event->delta()/240.0));
        view->scale(factor, factor);

        fixScaleFactor();
        event->accept();

    } else
        QGraphicsScene::wheelEvent(event);
}

void PatchScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        mouse_down_init = true;
    else
        mouse_down_init = false;
    mouse_rubberband = false;
    QGraphicsScene::mousePressEvent(event);
}

void PatchScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (mouse_down_init)
    {
        mouse_rubberband = (selectedItems().count() == 0);
        mouse_down_init = false;
    }

    if (mouse_rubberband)
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

        event->accept();
    } else
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

    mouse_down_init = false;
    mouse_rubberband = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

END_NAMESPACE_PATCHCANVAS
